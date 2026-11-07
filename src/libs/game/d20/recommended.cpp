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

#include "reone/game/d20/recommended.h"

#include <algorithm>

#include "reone/game/d20/class.h"
#include "reone/game/d20/feats.h"
#include "reone/game/d20/skills.h"
#include "reone/game/d20/spells.h"

namespace reone {

namespace game {

namespace recommended {

// Besides Force powers, Recommended chooses from the powers of this user type.
static constexpr int kRecommendedPowerUserType = 5;

// The level of a power whose class column is blank: no character reaches it.
static constexpr int kBlankPowerLevel = 255;

int getSkillPointGain(int skillPointBase, int intelligenceModifier, int characterLevel) {
    if (characterLevel == 1) {
        return std::max(1, skillPointBase / 2 + intelligenceModifier) * 4;
    }
    return std::max(1, (skillPointBase + intelligenceModifier) / 2);
}

// The lookup the original's three tables share. The fallback counts the
// priority-0 entries from fallbackStart and stops at the priority, so the
// skills and feats fall back to index priority and the powers to index
// priority - 1: the powers' counter starts one higher.
template <class T>
static std::optional<T> findByPriority(const std::vector<CreatureClass::Recommendation<T>> &entries, int priority, int fallbackStart) {
    std::optional<T> fallback;
    int counter = fallbackStart;
    for (const auto &entry : entries) {
        if (entry.priority == priority) {
            return entry.type;
        }
        if (entry.priority == 0 && counter != priority) {
            ++counter;
            fallback = entry.type;
        }
    }
    return fallback;
}

std::optional<SkillType> getRecommendedSkill(const CreatureClass &clazz, int priority) {
    return findByPriority(clazz.skillRecommendations(), priority, -1);
}

std::optional<FeatType> getRecommendedFeat(const CreatureClass &clazz, int priority) {
    return findByPriority(clazz.featRecommendations(), priority, -1);
}

static bool isRecommendedPowerType(const Spell &spell) {
    return spell.userType == kSpellUserTypeForcePower || spell.userType == kRecommendedPowerUserType;
}

// The spell table's rows, which the spells service loads every one of.
static std::vector<std::shared_ptr<Spell>> getSpellRows(const ISpells &spells) {
    std::vector<std::shared_ptr<Spell>> rows;
    for (int row = 0;; ++row) {
        auto spell = spells.get(static_cast<SpellType>(row));
        if (!spell) {
            break;
        }
        rows.push_back(std::move(spell));
    }
    return rows;
}

static std::optional<SpellType> getRecommendedPower(const std::vector<std::shared_ptr<Spell>> &rows, bool lightSide, int priority) {
    std::vector<CreatureClass::Recommendation<SpellType>> powers;
    for (const auto &spell : rows) {
        if (isRecommendedPowerType(*spell)) {
            powers.push_back({spell->type, lightSide ? spell->lightRecommendation : spell->darkRecommendation});
        }
    }
    return findByPriority(powers, priority, 0);
}

std::optional<SpellType> getRecommendedPower(const ISpells &spells, bool lightSide, int priority) {
    return getRecommendedPower(getSpellRows(spells), lightSide, priority);
}

// Takes one rank of the skill, when the points and the skill's cap allow it.
static bool incrementSkill(CreatureAttributes &attributes, SkillType skill, int &points, int rankLevel) {
    const auto &classLevels = attributes.classLevels();
    const bool classSkill = std::any_of(classLevels.begin(), classLevels.end(), [skill](const auto &classLevel) {
        return classLevel.first->isClassSkill(skill);
    });
    const int cost = classSkill ? 1 : 2;
    if (points < cost) {
        return false;
    }
    // The cap is a byte.
    int maxRank = (rankLevel + 3) & 0xff;
    if (!classSkill) {
        maxRank >>= 1;
    }
    const int rank = attributes.getSkillRank(skill);
    if (rank >= maxRank) {
        return false;
    }
    attributes.setSkillRank(skill, rank + 1);
    points -= cost;
    return true;
}

int recommendSkills(CreatureAttributes &attributes, int points, const SkillSpending &spending) {
    if (attributes.classLevels().empty()) {
        return points;
    }
    const CreatureClass &clazz = *attributes.classLevels().back().first;
    const int skillCount = static_cast<int>(clazz.skillRecommendations().size());
    for (int priority = 1; priority <= skillCount && points > 0; ++priority) {
        auto skill = getRecommendedSkill(clazz, priority);
        if (!skill) {
            continue;
        }
        if (!spending.playerCharacter) {
            auto rules = spending.skills.get(*skill);
            if (!rules || !rules->npcCanUse || (spending.droid && !rules->droidCanUse)) {
                continue;
            }
        }
        while (incrementSkill(attributes, *skill, points, spending.rankLevel)) {
        }
    }
    return points;
}

std::vector<FeatType> recommendFeats(const IFeats &feats, const FeatSpending &spending) {
    std::vector<FeatType> chosenOrder;
    std::set<FeatType> chosen;
    int remaining = spending.normalFeats + spending.bonusFeats;
    for (int priority = 1; priority <= spending.clazz.featRowCount() && remaining > 0; ++priority) {
        auto feat = getRecommendedFeat(spending.clazz, priority);
        if (!feat) {
            continue;
        }
        // The level's own counts bound the picks, not the screen's one more.
        FeatChoice choice {
            spending.attributes,
            spending.clazz,
            spending.normalFeats,
            spending.bonusFeats,
            spending.effectFeats,
            chosen,
            spending.tag,
            spending.tsl};
        auto choosable = feats.getChoosableFeats(choice);
        if (std::find(choosable.begin(), choosable.end(), *feat) == choosable.end()) {
            continue;
        }
        chosenOrder.push_back(*feat);
        chosen.insert(*feat);
        --remaining;
    }
    return chosenOrder;
}

static bool canChoosePower(
    const Spell &power,
    const CreatureAttributes &attributes,
    const CreatureClass &clazz,
    const std::set<SpellType> &chosen) {

    if (!isRecommendedPowerType(power)) {
        return false;
    }
    const auto &known = attributes.spellsForClass(clazz.type());
    if (std::find(known.begin(), known.end(), power.type) != known.end() || chosen.count(power.type) > 0) {
        return false;
    }
    for (auto prerequisite : power.prerequisites) {
        if (!attributes.hasSpell(prerequisite) && chosen.count(prerequisite) == 0) {
            return false;
        }
    }
    // The class's column is compared with the character level, not the
    // class level.
    return attributes.getAggregateLevel() >= power.getClassLevelRequirement(clazz.type()).value_or(kBlankPowerLevel);
}

std::vector<SpellType> recommendPowers(const ISpells &spells, const CreatureAttributes &attributes, const CreatureClass &clazz, int count, int goodEvil) {
    std::vector<SpellType> chosenOrder;
    if (!clazz.isJedi() || count <= 0) {
        return chosenOrder;
    }
    const auto rows = getSpellRows(spells);
    const bool lightSide = usesLightSidePowers(goodEvil);
    std::set<SpellType> chosen;
    for (int priority = 1; priority <= static_cast<int>(rows.size()) && static_cast<int>(chosen.size()) < count; ++priority) {
        auto power = getRecommendedPower(rows, lightSide, priority);
        if (!power) {
            continue;
        }
        const auto &spell = rows[static_cast<size_t>(*power)];
        if (canChoosePower(*spell, attributes, clazz, chosen)) {
            chosenOrder.push_back(*power);
            chosen.insert(*power);
        }
    }
    return chosenOrder;
}

CreatureAttributes chooseAutoLevel(const CreatureAttributes &attributes, const AutoLevelUp &character) {
    CreatureAttributes result(attributes);
    if (attributes.classLevels().empty()) {
        return result;
    }
    CreatureClass *clazz = attributes.classLevels().back().first;
    const int classLevel = attributes.classLevels().back().second + 1;
    auto levelled = [&result, clazz]() {
        CreatureAttributes copy(result);
        copy.addClassLevels(clazz, 1);
        return copy;
    };

    if (classLevel % 4 == 0) {
        if (auto ability = clazz->primaryAbility()) {
            result.setAbilityScore(*ability, result.getAbilityScore(*ability) + 1);
        }
    }

    // No unspent points are kept from an earlier level.
    const int points = getSkillPointGain(
        clazz->skillPointBase(),
        result.getAbilityModifier(Ability::Intelligence),
        attributes.getAggregateLevel() + 1);
    SkillSpending skills {character.skills, classLevel, character.playerCharacter, character.race == RacialType::Droid};
    recommendSkills(result, points, skills);

    {
        auto granted = levelled();
        character.feats.addGrantedFeats(granted, character.race, character.experience, character.tag, character.tsl, false);
        for (auto feat : granted.featOrder()) {
            result.addFeat(feat);
        }
    }
    {
        const auto chooser = levelled();
        FeatSpending feats {
            chooser,
            *clazz,
            clazz->getFeatGain(classLevel),
            clazz->getBonusFeatGain(classLevel),
            character.effectFeats,
            character.tag,
            character.tsl};
        for (auto feat : recommendFeats(character.feats, feats)) {
            result.addFeat(feat);
        }
    }

    // The level's granted powers, as the level-up screens add them, then
    // those a Jedi chooses.
    {
        auto granted = levelled();
        character.spells.addGrantedPowers(granted, character.tsl);
        for (auto power : granted.spellsForClass(clazz->type())) {
            if (!character.canLearnPower || character.canLearnPower(power, clazz->type())) {
                result.addSpell(power, clazz->type());
            }
        }
    }
    const auto chooser = levelled();
    for (auto power : recommendPowers(character.spells, chooser, *clazz, clazz->getPowerGain(classLevel), character.goodEvil)) {
        result.addSpell(power, clazz->type());
    }
    return result;
}

} // namespace recommended

} // namespace game

} // namespace reone
