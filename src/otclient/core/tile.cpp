/*
 * Copyright (c) 2010-2012 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "tile.h"
#include "item.h"
#include "thingstype.h"
#include "map.h"
#include "game.h"
#include "localplayer.h"
#include "effect.h"
#include <otclient/net/protocolgame.h>
#include <framework/graphics/fontmanager.h>

Tile::Tile(const Position& position)
{
    m_drawElevation = 0;
    m_position = position;
}

void Tile::draw(const Point& dest, float scaleFactor, int drawFlags)
{
    int drawElevation = 0;
    bool animate = drawFlags & Otc::DrawAnimations;

    // first bottom items
    if(drawFlags & Otc::DrawGround || drawFlags & Otc::DrawWalls || drawFlags & Otc::DrawGroundBorders) {
        for(const ThingPtr& thing : m_things) {
            if(!thing->isGround() && !thing->isGroundBorder() && !thing->isOnBottom())
                break;

            if((thing->isGround() && drawFlags & Otc::DrawGround) ||
               (thing->isGroundBorder() && drawFlags & Otc::DrawGroundBorders) ||
               (thing->isOnBottom() && drawFlags & Otc::DrawWalls))
                thing->draw(dest - drawElevation*scaleFactor, scaleFactor, animate);

            drawElevation += thing->getElevation();
            if(drawElevation > Otc::MAX_ELEVATION)
                drawElevation = Otc::MAX_ELEVATION;
        }
    }

    // now common items in reverse order
    if(drawFlags & Otc::DrawCommonItems) {
        for(auto it = m_things.rbegin(); it != m_things.rend(); ++it) {
            const ThingPtr& thing = *it;
            if(thing->isOnTop() || thing->isOnBottom() || thing->isGroundBorder() || thing->isGround() || thing->asCreature())
                break;
            thing->draw(dest - drawElevation*scaleFactor, scaleFactor, animate);

            drawElevation += thing->getElevation();
            if(drawElevation > Otc::MAX_ELEVATION)
                drawElevation = Otc::MAX_ELEVATION;
        }
    }

    // creatures
    if(drawFlags & Otc::DrawCreatures) {
        if(animate) {
            for(const CreaturePtr& creature : m_walkingCreatures) {
                creature->draw(Point(dest.x + ((creature->getPosition().x - m_position.x)*Otc::TILE_PIXELS - drawElevation)*scaleFactor,
                                    dest.y + ((creature->getPosition().y - m_position.y)*Otc::TILE_PIXELS - drawElevation)*scaleFactor), scaleFactor, animate);

            }
        }

        for(auto it = m_things.rbegin(); it != m_things.rend(); ++it) {
            CreaturePtr creature = (*it)->asCreature();
            if(creature && (!creature->isWalking() || !animate))
                creature->draw(dest - drawElevation, scaleFactor, animate);
        }
    }

    // effects
    if(drawFlags & Otc::DrawEffects) {
        for(const EffectPtr& effect : m_effects)
            effect->draw(dest, scaleFactor, animate);
    }

    // top items
    if(drawFlags & Otc::DrawWalls) {
        for(const ThingPtr& thing : m_things) {
            if(thing->isOnTop())
                thing->draw(dest - drawElevation, scaleFactor, animate);
        }
    }
}

void Tile::clean()
{
    m_things.clear();
    m_effects.clear();
}

void Tile::addWalkingCreature(const CreaturePtr& creature)
{
    m_walkingCreatures.push_back(creature);
}

void Tile::removeWalkingCreature(const CreaturePtr& creature)
{
    auto it = std::find(m_walkingCreatures.begin(), m_walkingCreatures.end(), creature);
    if(it != m_walkingCreatures.end())
        m_walkingCreatures.erase(it);
}

ThingPtr Tile::addThing(const ThingPtr& thing, int stackPos)
{
    if(!thing)
        return nullptr;

    thing->setPosition(m_position);

    if(EffectPtr effect = thing->asEffect()) {
        m_effects.push_back(effect);
        return nullptr;
    }

    if(stackPos < 0) {
        stackPos = 0;
        int priority = thing->getStackPriority();
        for(stackPos = 0; stackPos < (int)m_things.size(); ++stackPos) {
            int otherPriority = m_things[stackPos]->getStackPriority();
            if(otherPriority > priority || (otherPriority == priority && otherPriority == 5))
                break;
        }
    } else if(stackPos > (int)m_things.size())
        stackPos = m_things.size();

    ThingPtr oldObject;
    if(stackPos < (int)m_things.size())
        oldObject = m_things[stackPos];
    m_things.insert(m_things.begin() + stackPos, thing);

    return oldObject;
}

bool Tile::removeThing(const ThingPtr& thing)
{
    if(!thing)
        return false;

    bool removed = false;

    if(EffectPtr effect = thing->asEffect()) {
        auto it = std::find(m_effects.begin(), m_effects.end(), effect);
        if(it != m_effects.end()) {
            m_effects.erase(it);
            removed = true;
        }
    } else {
        auto it = std::find(m_things.begin(), m_things.end(), thing);
        if(it != m_things.end()) {
            m_things.erase(it);
            removed = true;
        }
    }

    // reset values managed by this tile
    if(removed) {
        //thing->setDrawOffset(0);
        //thing->setStackpos(0);
    }

    return removed;
}

ThingPtr Tile::getThing(int stackPos)
{
    if(stackPos >= 0 && stackPos < (int)m_things.size())
        return m_things[stackPos];
    return nullptr;
}

int Tile::getThingStackpos(const ThingPtr& thing)
{
    for(uint stackpos = 0; stackpos < m_things.size(); ++stackpos)
        if(thing == m_things[stackpos])
            return stackpos;
    return -1;
}

ThingPtr Tile::getTopThing()
{
    if(isEmpty())
        return nullptr;

    return m_things[m_things.size() - 1];
}

std::vector<CreaturePtr> Tile::getCreatures()
{
    std::vector<CreaturePtr> creatures;
    for(const ThingPtr& thing : m_things) {
        if(CreaturePtr creature = thing->asCreature())
            creatures.push_back(creature);
    }
    return creatures;
}

ItemPtr Tile::getGround()
{
    ThingPtr firstObject = getThing(0);
    if(!firstObject)
        return nullptr;
    if(firstObject->isGround())
        return firstObject->asItem();
    return nullptr;
}

int Tile::getGroundSpeed()
{
    int groundSpeed = 100;
    if(ItemPtr ground = getGround())
        groundSpeed = ground->getGroundSpeed();
    return groundSpeed;
}

ThingPtr Tile::getTopLookThing()
{
    if(isEmpty())
        return nullptr;

    for(uint i = 0; i < m_things.size(); ++i) {
        ThingPtr thing = m_things[i];
        if(!thing->isIgnoreLook() && (!thing->isGround() && !thing->isGroundBorder() && !thing->isOnBottom() && !thing->isOnTop()))
            return thing;
    }

    return m_things[0];
}

ThingPtr Tile::getTopUseThing()
{
    if(isEmpty())
        return nullptr;

    for(uint i = 0; i < m_things.size(); ++i) {
        ThingPtr thing = m_things[i];
        if(thing->isForceUse() || (!thing->isGround() && !thing->isGroundBorder() && !thing->isOnBottom() && !thing->isOnTop() && !thing->asCreature()))
            return thing;
    }

    return m_things[0];
}

CreaturePtr Tile::getTopCreature()
{
    CreaturePtr creature;
    for(uint i = 0; i < m_things.size(); ++i) {
        ThingPtr thing = m_things[i];
        if(thing->asLocalPlayer()) // return local player if there is no other creature
            creature = thing->asCreature();
        else if(thing->asCreature() && !thing->asLocalPlayer())
            return thing->asCreature();
    }
    return creature;
}

ThingPtr Tile::getTopMoveThing()
{
    if(isEmpty())
        return nullptr;

    for(uint i = 0; i < m_things.size(); ++i) {
        ThingPtr thing = m_things[i];
        if(!thing->isGround() && !thing->isGroundBorder() && !thing->isOnBottom() && !thing->isOnTop() && !thing->asCreature()) {
            if(i > 0 && thing->isNotMoveable())
                return m_things[i-1];
            return thing;
        }
    }

    return m_things[0];
}

ThingPtr Tile::getTopMultiUseThing()
{
    // this is related to classic controls, getting top item, forceuse or creature
    if(isEmpty())
        return nullptr;

    for(uint i = 0; i < m_things.size(); ++i) {
        ThingPtr thing = m_things[i];
        if(thing->isForceUse() || (!thing->isGround() && !thing->isGroundBorder() && !thing->isOnBottom() && !thing->isOnTop())) {
            if(i > 0 && thing->isFluid())
                return m_things[i-1];
            return thing;
        }
    }

    return m_things[0];
}

bool Tile::isWalkable()
{
    if(!getGround())
        return false;

    for(const ThingPtr& thing : m_things) {
        if(thing->isNotWalkable())
            return false;

        if(CreaturePtr creature = thing->asCreature()) {
            if(!creature->getPassable())
                return false;
        }
    }
    return true;
}

bool Tile::isFullGround()
{
    ItemPtr ground = getGround();
    if(ground && ground->isFullGround())
        return true;
    return false;
}

bool Tile::isFullyOpaque()
{
    ThingPtr firstObject = getThing(0);
    return firstObject && firstObject->isFullGround();
}

bool Tile::isLookPossible()
{
    for(const ThingPtr& thing : m_things) {
        if(thing->blocksProjectile())
            return false;
    }
    return true;
}

bool Tile::isClickable()
{
    bool hasGround = false;
    bool hasOnBottom = false;
    bool hasIgnoreLook = false;
    for(const ThingPtr& thing : m_things) {
        if(thing->isGround())
            hasGround = true;
        if(thing->isOnBottom())
            hasOnBottom = true;
        if(thing->isIgnoreLook())
            hasIgnoreLook = true;

        if((hasGround || hasOnBottom) && !hasIgnoreLook)
            return true;
    }
    return false;
}

bool Tile::isEmpty()
{
    return m_things.size() == 0;
}

bool Tile::mustHookEast()
{
    for(const ThingPtr& thing : m_things)
        if(thing->isHookEast())
            return true;
    return false;
}

bool Tile::mustHookSouth()
{
    for(const ThingPtr& thing : m_things)
        if(thing->isHookSouth())
            return true;
    return false;
}

bool Tile::hasCreature()
{
    for(const ThingPtr& thing : m_things)
        if(thing->asCreature())
            return true;
    return false;
}

bool Tile::limitsFloorsView()
{
    // ground and walls limits the view
    ThingPtr firstThing = getThing(0);
    if(firstThing && !firstThing->isDontHide() && (firstThing->isGround() || firstThing->isOnBottom()))
        return true;
    return false;
}
