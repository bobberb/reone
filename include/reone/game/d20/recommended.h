/*
 * Copyright (c) 2026 The reone project contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "../types.h"

#include "attributes.h"

namespace reone {

namespace game {

class CreatureClass;
class IFeats;
class ISkills;
class ISpells;

/**
 * The choices the Recommended buttons of the level-up and character
 * generation screens make, and the character sheet's Auto Level-Up.
 */
namespace recommended {

/** The alignment above which a character takes the light side powers. */
constexpr int kLightSideGoodEvil = 40;

/**
 * @return the skill points a character gains at the character level: at
 *         level 1 four times the larger of 1 and half the class's base plus
 *         the Intelligence modifier, at a later level the larger of 1 and half
 *         the base and the modifier together, rounded toward zero
 */
int getSkillPointGain(int skillPointBase, int intelligenceModifier, int characterLevel);

/**
 * @return the skill the class recommends at the priority: the first skill
 *         with that priority, or else, of the skills with priority 0 in row
 *         order, the one at the priority as a 0-based index, or the last one
 *         past their end; none when no skill has priority 0
 */
std::optional<SkillType> getRecommendedSkill(const CreatureClass &clazz, int priority);

/** @return the feat the class recommends at the priority, chosen as getRecommendedSkill chooses */
std::optional<FeatType> getRecommendedFeat(const CreatureClass &clazz, int priority);

/**
 * @return the Force power recommended at the priority, among the powers
 *         (user type 1 or 5): the first with that light or dark side
 *         priority, or else, of the powers with priority 0 in row order, the
 *         one at the priority less one as a 0-based index, or the last one
 *         past their end; none when no power has priority 0
 */
std::optional<SpellType> getRecommendedPower(const ISpells &spells, bool lightSide, int priority);

/** @return whether a character of the alignment takes the light side powers: above 40 */
inline bool usesLightSidePowers(int goodEvil) {
    return goodEvil > kLightSideGoodEvil;
}

/** A character spending skill points as Recommended does. */
struct SkillSpending {
    const ISkills &skills;
    /** The level whose cap applies: a rank up to the level plus 3, half that for a cross-class skill. */
    int rankLevel {1};
    /** False for a character other than the player character, which spends only on the skills an NPC can use. */
    bool playerCharacter {true};
    /** True for a droid, which as an NPC spends only on the skills a droid can use. */
    bool droid {false};
};

/**
 * Spends the skill points by priority, from 1 to the number of skills while
 * points are left: each recommended skill takes as many ranks as the points
 * and its cap allow. A class skill of any of the character's classes costs a
 * point a rank, another skill two. The priorities are the newest class's.
 *
 * @return the points left
 */
int recommendSkills(CreatureAttributes &attributes, int points, const SkillSpending &spending);

/** A character choosing feats as Recommended does. */
struct FeatSpending {
    /** The character as it is with the level it gains, with the level's granted feats. */
    const CreatureAttributes &attributes;
    /** The class the level is gained in, whose priorities and lists apply. */
    const CreatureClass &clazz;
    /** The normal and the bonus feats to choose. */
    int normalFeats {0};
    int bonusFeats {0};
    /** The feats the character's effects give it. */
    const std::set<FeatType> &effectFeats;
    std::string tag;
    bool tsl {false};
};

/**
 * Chooses feats by priority, from 1 to the rows of the feat table, until
 * the normal and the bonus feats are chosen: each recommended feat the
 * character can choose, counting those chosen before it, is chosen.
 *
 * @return the chosen feats, in the order they were chosen
 */
std::vector<FeatType> recommendFeats(const IFeats &feats, const FeatSpending &spending);

/**
 * Chooses Force powers by light or dark side priority, from 1 to the rows of
 * the spell table, until \p count are chosen; none for a class other than a
 * Jedi class. A power can be chosen when the newest class does not know it,
 * it has not been chosen, each of its prerequisites is known or chosen and
 * the character level reaches the class's level for it.
 *
 * @param attributes the character as it is with the level it gains
 * @return the chosen powers, in the order they were chosen
 */
std::vector<SpellType> recommendPowers(const ISpells &spells, const CreatureAttributes &attributes, const CreatureClass &clazz, int count, int goodEvil);

/** A character the character sheet's Auto Level-Up levels. */
struct AutoLevelUp {
    const IFeats &feats;
    const ISkills &skills;
    const ISpells &spells;
    RacialType race {RacialType::Human};
    int experience {0};
    std::string tag;
    bool tsl {false};
    bool playerCharacter {true};
    int goodEvil {50};
    std::set<FeatType> effectFeats;
    /** Whether a power the level grants is learned; every one is when unset. */
    std::function<bool(SpellType, ClassType)> canLearnPower;
};

/**
 * Chooses one level in the newest class as Auto Level-Up does: the primary
 * ability gains a point at a class level that is a multiple of 4; the
 * level's skill points are spent as recommended, capped by the class level;
 * the level's granted feats are added and its normal and bonus feats chosen
 * together as recommended; and a Jedi chooses the level's powers as
 * recommended.
 *
 * @return the attributes to apply with the level: the new class level is not
 *         among them, as Creature::applyLevelUp adds it
 */
CreatureAttributes chooseAutoLevel(const CreatureAttributes &attributes, const AutoLevelUp &character);

} // namespace recommended

} // namespace game

} // namespace reone
