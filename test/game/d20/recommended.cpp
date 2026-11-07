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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "reone/game/d20/attributes.h"
#include "reone/game/d20/classes.h"
#include "reone/game/d20/feats.h"
#include "reone/game/d20/races.h"
#include "reone/game/d20/recommended.h"
#include "reone/game/d20/skills.h"
#include "reone/game/d20/spells.h"
#include "reone/resource/2da.h"

#include "../../fixtures/game.h"
#include "../../fixtures/resource.h"

using namespace reone;
using namespace reone::game;
using namespace reone::resource;
using namespace testing;

namespace {

using Row = std::unordered_map<std::string, std::string>;

std::shared_ptr<TwoDA> makeTable(
    const std::vector<std::string> &columns,
    const std::vector<std::vector<std::string>> &rows) {
    TwoDA::Builder builder;
    builder.columns(columns);
    for (const auto &row : rows) {
        builder.row(row);
    }
    return std::shared_ptr<TwoDA>(builder.build());
}

std::shared_ptr<TwoDA> makeSparseTable(
    const std::vector<std::string> &columns,
    int rowCount,
    const std::unordered_map<int, Row> &values) {
    TwoDA::Builder builder;
    builder.columns(columns);
    for (int row = 0; row < rowCount; ++row) {
        std::vector<std::string> cells(columns.size());
        auto maybeRow = values.find(row);
        if (maybeRow != values.end()) {
            for (size_t column = 0; column < columns.size(); ++column) {
                auto maybeValue = maybeRow->second.find(columns[column]);
                if (maybeValue != maybeRow->second.end()) {
                    cells[column] = maybeValue->second;
                }
            }
        }
        builder.row(std::move(cells));
    }
    return std::shared_ptr<TwoDA>(builder.build());
}

// The KotOR classes.2da rows of the classes the tests level, and a test class
// whose skills table has Persuade at priority 0.
std::shared_ptr<TwoDA> makeClassesTable() {
    const std::vector<std::string> columns = {
        "name", "description", "hitdie", "skillpointbase", "str", "dex", "con", "wis", "int", "cha",
        "skillstable", "savingthrowtable", "attackbonustable", "featstable", "featgain", "spellgaintable", "primaryabil"};
    std::vector<std::vector<std::string>> rows(17, std::vector<std::string>(columns.size()));
    rows[static_cast<int>(ClassType::Soldier)] = {"134", "1308", "10", "2", "16", "14", "14", "12", "10", "10", "SOL", "save", "attack", "SOL", "SOL", "", "STR"};
    rows[static_cast<int>(ClassType::Scoundrel)] = {"135", "1310", "6", "8", "10", "16", "10", "12", "14", "14", "SCD", "save", "attack", "SCD", "SCD", "", "DEX"};
    rows[static_cast<int>(ClassType::JediGuardian)] = {"353", "1311", "10", "2", "14", "14", "14", "14", "12", "10", "JGD", "save", "attack", "", "", "", "STR"};
    // Not a KotOR class: the Minion row stands in for a class with Persuade at 0.
    rows[static_cast<int>(ClassType::Minion)] = {"1", "2", "10", "2", "10", "10", "10", "10", "10", "10", "TST", "save", "attack", "", "", "", ""};
    return makeTable(columns, rows);
}

// The KotOR skills.2da, with the test class's columns.
std::shared_ptr<TwoDA> makeSkillsTable() {
    const std::vector<std::string> columns = {
        "label", "npccanuse", "droidcanuse",
        "scd_class", "scd_reco", "sol_class", "sol_reco", "jgd_class", "jgd_reco", "tst_class", "tst_reco"};
    return makeTable(columns, {
                                  {"ComputerUse", "1", "1", "0", "7", "0", "6", "0", "5", "0", "1"},
                                  {"Demolitions", "1", "1", "1", "2", "1", "3", "0", "4", "0", "2"},
                                  {"Stealth", "1", "0", "1", "1", "0", "7", "0", "7", "0", "3"},
                                  {"Awareness", "1", "1", "1", "4", "1", "2", "1", "2", "0", "4"},
                                  {"Persuade", "0", "0", "1", "8", "0", "", "1", "", "1", "0"},
                                  {"Repair", "1", "1", "0", "6", "0", "4", "0", "3", "0", "5"},
                                  {"Security", "1", "1", "1", "3", "0", "5", "0", "6", "0", "6"},
                                  {"TreatInjury", "1", "0", "0", "5", "1", "1", "1", "1", "0", "7"},
                              });
}

// The KotOR feat.2da cells the choice of a Scoundrel's feats reads.
std::shared_ptr<TwoDA> makeFeatTable() {
    const std::vector<std::string> columns = {
        "name", "mincharlevel", "minattackbonus", "minstr", "mindex", "minint", "minwis",
        "prereqfeat1", "prereqfeat2", "orreqfeat0", "orreqfeat1", "orreqfeat2", "orreqfeat3", "orreqfeat4",
        "reqskill", "scd_list", "scd_granted", "scd_recom"};
    std::unordered_map<int, Row> rows {
        {1, {{"name", "148"}, {"mincharlevel", "4"}, {"prereqfeat1", "55"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {2, {{"name", "149"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {3, {{"name", "150"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {4, {{"name", "153"}, {"prereqfeat1", "5"}, {"prereqfeat2", "6"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {5, {{"name", "151"}, {"scd_list", "3"}, {"scd_granted", "1"}}},
        {6, {{"name", "152"}, {"prereqfeat1", "5"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {7, {{"name", "154"}, {"scd_list", "1"}, {"scd_granted", "-1"}}},
        {8, {{"name", "155"}, {"scd_list", "3"}, {"scd_granted", "1"}}},
        {9, {{"name", "157"}, {"mincharlevel", "4"}, {"prereqfeat1", "3"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {10, {{"name", "158"}, {"scd_list", "1"}, {"scd_granted", "-1"}}},
        {11, {{"name", "160"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {12, {{"name", "161"}, {"scd_list", "1"}, {"scd_granted", "-1"}, {"scd_recom", "3"}}},
        {13, {{"name", "162"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {14, {{"name", "164"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {15, {{"name", "165"}, {"mincharlevel", "4"}, {"prereqfeat1", "14"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {16, {{"name", "166"}, {"mincharlevel", "8"}, {"prereqfeat1", "14"}, {"prereqfeat2", "15"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {17, {{"name", "167"}, {"mincharlevel", "4"}, {"prereqfeat1", "28"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {18, {{"name", "168"}, {"mincharlevel", "4"}, {"prereqfeat1", "29"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {19, {{"name", "169"}, {"mincharlevel", "4"}, {"prereqfeat1", "8"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {20, {{"name", "170"}, {"mincharlevel", "4"}, {"prereqfeat1", "31"}, {"scd_list", "0"}, {"scd_granted", "-1"}, {"scd_recom", "4"}}},
        {21, {{"name", "171"}, {"mincharlevel", "4"}, {"prereqfeat1", "13"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {22, {{"name", "172"}, {"mincharlevel", "8"}, {"prereqfeat1", "13"}, {"prereqfeat2", "21"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {23, {{"prereqfeat1", "0"}}},
        {24, {{"name", "174"}, {"mincharlevel", "8"}, {"prereqfeat1", "55"}, {"prereqfeat2", "1"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {25, {{"name", "175"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {26, {{"name", "176"}, {"mincharlevel", "8"}, {"prereqfeat1", "30"}, {"prereqfeat2", "92"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {27, {{"name", "177"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {28, {{"name", "178"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {29, {{"name", "179"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {30, {{"name", "180"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {31, {{"name", "181"}, {"scd_list", "3"}, {"scd_granted", "1"}, {"scd_recom", "2"}}},
        {32, {{"name", "182"}, {"prereqfeat1", "39"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {33, {{"name", "183"}, {"prereqfeat1", "40"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {34, {{"name", "184"}, {"prereqfeat1", "41"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {35, {{"name", "185"}, {"prereqfeat1", "42"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {36, {{"name", "186"}, {"prereqfeat1", "43"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {37, {{"name", "187"}, {"prereqfeat1", "44"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {38, {{"name", "188"}, {"prereqfeat1", "45"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {39, {{"name", "189"}, {"scd_list", "3"}, {"scd_granted", "1"}}},
        {40, {{"name", "190"}, {"scd_list", "3"}, {"scd_granted", "1"}}},
        {41, {{"name", "191"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {42, {{"name", "192"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {43, {{"name", "193"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {44, {{"name", "194"}, {"scd_list", "3"}, {"scd_granted", "1"}}},
        {45, {{"name", "195"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {46, {{"name", "196"}, {"mincharlevel", "4"}, {"prereqfeat1", "39"}, {"prereqfeat2", "32"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {47, {{"name", "197"}, {"mincharlevel", "4"}, {"prereqfeat1", "40"}, {"prereqfeat2", "33"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {48, {{"name", "198"}, {"prereqfeat1", "41"}, {"prereqfeat2", "34"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {49, {{"name", "199"}, {"mincharlevel", "4"}, {"prereqfeat1", "42"}, {"prereqfeat2", "35"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {50, {{"name", "200"}, {"mincharlevel", "4"}, {"prereqfeat1", "43"}, {"prereqfeat2", "36"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {51, {{"name", "201"}, {"mincharlevel", "4"}, {"prereqfeat1", "44"}, {"prereqfeat2", "37"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {52, {{"name", "202"}, {"prereqfeat1", "45"}, {"prereqfeat2", "38"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {53, {{"name", "203"}, {"mincharlevel", "8"}, {"prereqfeat1", "11"}, {"prereqfeat2", "91"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {54, {{"name", "147"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {55, {{"name", "156"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {56, {{"name", "159"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {57, {{"name", "163"}, {"prereqfeat1", "56"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {58, {{"name", "204"}, {"mincharlevel", "0"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {60, {{"name", "1032"}, {"scd_list", "3"}, {"scd_granted", "1"}}},
        {61, {{"name", "1033"}, {"prereqfeat1", "60"}, {"scd_list", "3"}, {"scd_granted", "3"}}},
        {62, {{"name", "1034"}, {"prereqfeat1", "60"}, {"prereqfeat2", "61"}, {"scd_list", "3"}, {"scd_granted", "5"}}},
        {63, {{"name", "1035"}, {"scd_list", "3"}, {"scd_granted", "7"}}},
        {64, {{"name", "1036"}, {"prereqfeat1", "63"}, {"scd_list", "3"}, {"scd_granted", "9"}}},
        {65, {{"name", "1037"}, {"prereqfeat1", "63"}, {"prereqfeat2", "64"}, {"scd_list", "3"}, {"scd_granted", "11"}}},
        {66, {{"name", "1038"}, {"scd_list", "3"}, {"scd_granted", "13"}}},
        {67, {{"name", "1039"}, {"prereqfeat1", "66"}, {"scd_list", "3"}, {"scd_granted", "15"}}},
        {68, {{"name", "1040"}, {"prereqfeat1", "66"}, {"prereqfeat2", "67"}, {"scd_list", "3"}, {"scd_granted", "17"}}},
        {69, {{"name", "1041"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {70, {{"name", "1042"}, {"mincharlevel", "0"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {71, {{"name", "1043"}, {"mincharlevel", "0"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {72, {{"name", "1044"}, {"mincharlevel", "0"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {73, {{"name", "1045"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {74, {{"name", "1046"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {75, {{"name", "1047"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {76, {{"name", "1048"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {77, {{"name", "1049"}, {"mincharlevel", "8"}, {"prereqfeat1", "31"}, {"prereqfeat2", "20"}, {"scd_list", "0"}, {"scd_granted", "-1"}, {"scd_recom", "7"}}},
        {78, {{"name", "1050"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {79, {{"name", "1051"}, {"prereqfeat1", "78"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {80, {{"name", "1052"}, {"prereqfeat1", "78"}, {"prereqfeat2", "79"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {81, {{"name", "1190"}, {"mincharlevel", "8"}, {"prereqfeat1", "8"}, {"prereqfeat2", "19"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {82, {{"name", "1191"}, {"mincharlevel", "8"}, {"prereqfeat1", "29"}, {"prereqfeat2", "18"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {83, {{"name", "1192"}, {"mincharlevel", "8"}, {"prereqfeat1", "28"}, {"prereqfeat2", "17"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {84, {{"name", "1193"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {85, {{"name", "1194"}, {"mincharlevel", "8"}, {"prereqfeat1", "3"}, {"prereqfeat2", "9"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {86, {{"name", "1195"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {87, {{"name", "1196"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {88, {{"name", "1197"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {89, {{"name", "1198"}, {"mincharlevel", "4"}, {"prereqfeat1", "88"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {90, {{"name", "1199"}, {"mincharlevel", "8"}, {"prereqfeat1", "88"}, {"prereqfeat2", "89"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {91, {{"name", "1200"}, {"mincharlevel", "4"}, {"prereqfeat1", "11"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {92, {{"name", "1201"}, {"mincharlevel", "4"}, {"prereqfeat1", "30"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {93, {{"name", "32077"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {94, {{"name", "32079"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {95, {{"name", "38526"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {96, {{"name", "38527"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {97, {{"name", "38528"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {98, {{"name", "38530"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {99, {{"name", "38529"}, {"prereqfeat1", "98"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {100, {{"name", "38531"}, {"prereqfeat1", "98"}, {"prereqfeat2", "88"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {101, {{"name", "280"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {102, {{"name", "38532"}, {"prereqfeat1", "101"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {103, {{"name", "38533"}, {"prereqfeat1", "101"}, {"prereqfeat2", "102"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {104, {{"name", "38584"}, {"scd_list", "3"}, {"scd_granted", "1"}}},
        {105, {{"name", "38585"}, {"prereqfeat1", "104"}, {"scd_list", "3"}, {"scd_granted", "6"}}},
        {106, {{"name", "38586"}, {"prereqfeat1", "104"}, {"prereqfeat2", "105"}, {"scd_list", "3"}, {"scd_granted", "12"}}},
        {107, {{"name", "38587"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {108, {{"name", "38588"}, {"prereqfeat1", "107"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {109, {{"name", "38589"}, {"prereqfeat1", "107"}, {"prereqfeat2", "108"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {110, {{"name", "38590"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {111, {{"name", "38591"}, {"prereqfeat1", "110"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {112, {{"name", "38592"}, {"prereqfeat1", "110"}, {"prereqfeat2", "111"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {113, {{"name", "41858"}, {"scd_list", "0"}, {"scd_granted", "-1"}, {"scd_recom", "1"}}},
        {114, {{"name", "41859"}, {"mincharlevel", "4"}, {"prereqfeat1", "113"}, {"scd_list", "0"}, {"scd_granted", "-1"}, {"scd_recom", "5"}}},
        {115, {{"name", "41860"}, {"mincharlevel", "8"}, {"prereqfeat1", "113"}, {"prereqfeat2", "114"}, {"scd_list", "0"}, {"scd_granted", "-1"}, {"scd_recom", "8"}}},
        {116, {{"name", "41861"}, {"scd_list", "4"}, {"scd_granted", "-1"}}},
        {117, {{"name", "41994"}, {"mincharlevel", "4"}, {"prereqfeat1", "7"}, {"scd_list", "1"}, {"scd_granted", "-1"}}},
        {118, {{"name", "41993"}, {"mincharlevel", "8"}, {"prereqfeat1", "7"}, {"prereqfeat2", "117"}, {"scd_list", "1"}, {"scd_granted", "-1"}}},
        {119, {{"name", "42001"}, {"mincharlevel", "4"}, {"prereqfeat1", "12"}, {"scd_list", "1"}, {"scd_granted", "-1"}, {"scd_recom", "6"}}},
        {120, {{"name", "42002"}, {"mincharlevel", "8"}, {"prereqfeat1", "12"}, {"prereqfeat2", "119"}, {"scd_list", "1"}, {"scd_granted", "-1"}, {"scd_recom", "9"}}},
        {121, {{"name", "41995"}, {"mincharlevel", "4"}, {"prereqfeat1", "10"}, {"scd_list", "1"}, {"scd_granted", "-1"}}},
        {122, {{"name", "41996"}, {"mincharlevel", "8"}, {"prereqfeat1", "10"}, {"prereqfeat2", "121"}, {"scd_list", "1"}, {"scd_granted", "-1"}}},
        {123, {{"name", "41986"}, {"mincharlevel", "4"}, {"prereqfeat1", "84"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
        {124, {{"name", "41987"}, {"mincharlevel", "8"}, {"prereqfeat1", "84"}, {"prereqfeat2", "123"}, {"scd_list", "0"}, {"scd_granted", "-1"}}},
    };
    return makeSparseTable(columns, 125, rows);
}

// The KotOR featgain.2da Scoundrel columns.
std::shared_ptr<TwoDA> makeFeatGainTable() {
    const std::vector<std::string> regular {"1", "1", "0", "0", "1", "0", "0", "1", "0", "0", "1", "0", "0", "1", "0", "0", "1", "0", "0", "1"};
    std::vector<std::vector<std::string>> rows;
    for (const auto &gain : regular) {
        rows.push_back({gain, "0"});
    }
    return makeTable({"scd_reg", "scd_bon"}, rows);
}

// The KotOR spells.2da cells the choice of powers reads, for its Force
// powers; the other rows are blank.
std::shared_ptr<TwoDA> makeSpellsTable() {
    const std::vector<std::string> columns = {
        "usertype", "prerequisites", "guardian", "consular", "sentinel", "light_recom", "dark_recom"};
    std::unordered_map<int, Row> rows {
        // FORCE_POWER_LIGHT_SABER_THROW_ADVANCED
        {4, {{"usertype", "1"}, {"prerequisites", "49"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"light_recom", "9"}, {"dark_recom", "22"}}},
        // FORCE_POWER_AFFECT_MIND
        {6, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"light_recom", "28"}, {"dark_recom", "29"}}},
        // FORCE_POWER_AFFLICTION
        {7, {{"usertype", "1"}, {"prerequisites", "45"}, {"guardian", "6"}, {"consular", "6"}, {"sentinel", "6"}, {"dark_recom", "10"}}},
        // FORCE_POWER_SPEED_BURST
        {8, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"light_recom", "16"}, {"dark_recom", "18"}}},
        // FORCE_POWER_CHOKE
        {9, {{"usertype", "1"}, {"prerequisites", "50"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"dark_recom", "5"}}},
        // FORCE_POWER_CURE
        {10, {{"usertype", "1"}, {"guardian", "6"}, {"consular", "6"}, {"sentinel", "6"}, {"light_recom", "6"}}},
        // FORCE_POWER_DEATH_FIELD
        {11, {{"usertype", "1"}, {"prerequisites", "15"}, {"guardian", "18"}, {"consular", "18"}, {"sentinel", "18"}, {"dark_recom", "8"}}},
        // FORCE_POWER_DROID_DISABLE
        {12, {{"usertype", "1"}, {"prerequisites", "47"}, {"guardian", "6"}, {"consular", "6"}, {"sentinel", "6"}, {"light_recom", "11"}}},
        // FORCE_POWER_DROID_DESTROY
        {13, {{"usertype", "1"}, {"prerequisites", "47_12"}, {"guardian", "12"}, {"consular", "12"}, {"sentinel", "12"}, {"light_recom", "12"}}},
        // FORCE_POWER_DOMINATE
        {14, {{"usertype", "1"}, {"prerequisites", "6"}, {"guardian", "6"}, {"consular", "6"}, {"sentinel", "6"}, {"light_recom", "29"}, {"dark_recom", "30"}}},
        // FORCE_POWER_DRAIN_LIFE
        {15, {{"usertype", "1"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"dark_recom", "7"}}},
        // FORCE_POWER_FEAR
        {16, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"dark_recom", "12"}}},
        // FORCE_POWER_FORCE_ARMOR
        {17, {{"usertype", "1"}, {"prerequisites", "18_24"}, {"guardian", "12"}, {"consular", "12"}, {"sentinel", "12"}, {"light_recom", "15"}}},
        // FORCE_POWER_FORCE_AURA
        {18, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"light_recom", "13"}}},
        // FORCE_POWER_FORCE_BREACH
        {19, {{"usertype", "1"}, {"prerequisites", "48"}, {"guardian", "15"}, {"consular", "15"}, {"sentinel", "15"}, {"light_recom", "27"}, {"dark_recom", "24"}}},
        // FORCE_POWER_FORCE_IMMUNITY
        {20, {{"usertype", "1"}, {"prerequisites", "41"}, {"guardian", "15"}, {"consular", "15"}, {"sentinel", "15"}, {"light_recom", "25"}, {"dark_recom", "26"}}},
        // FORCE_POWER_FORCE_VALOR
        {22, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"light_recom", "19"}}},
        // FORCE_POWER_FORCE_PUSH
        {23, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"light_recom", "4"}, {"dark_recom", "15"}}},
        // FORCE_POWER_FORCE_SHIELD
        {24, {{"usertype", "1"}, {"prerequisites", "18"}, {"guardian", "6"}, {"consular", "6"}, {"sentinel", "6"}, {"light_recom", "14"}}},
        // FORCE_POWER_FORCE_STORM
        {25, {{"usertype", "1"}, {"prerequisites", "43_35"}, {"guardian", "18"}, {"consular", "18"}, {"sentinel", "18"}, {"dark_recom", "3"}}},
        // FORCE_POWER_FORCE_WAVE
        {26, {{"usertype", "1"}, {"prerequisites", "23_27"}, {"guardian", "15"}, {"consular", "15"}, {"sentinel", "15"}, {"light_recom", "6"}, {"dark_recom", "17"}}},
        // FORCE_POWER_FORCE_WHIRLWIND
        {27, {{"usertype", "1"}, {"prerequisites", "23"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"light_recom", "5"}, {"dark_recom", "16"}}},
        // FORCE_POWER_HEAL
        {28, {{"usertype", "1"}, {"prerequisites", "10"}, {"guardian", "12"}, {"consular", "12"}, {"sentinel", "12"}, {"light_recom", "7"}}},
        // FORCE_POWER_STASIS
        {29, {{"usertype", "1"}, {"prerequisites", "46"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"light_recom", "2"}}},
        // FORCE_POWER_HORROR
        {30, {{"usertype", "1"}, {"prerequisites", "16"}, {"guardian", "6"}, {"consular", "6"}, {"sentinel", "6"}, {"dark_recom", "13"}}},
        // FORCE_POWER_INSANITY
        {31, {{"usertype", "1"}, {"prerequisites", "16_30"}, {"guardian", "12"}, {"consular", "12"}, {"sentinel", "12"}, {"dark_recom", "14"}}},
        // FORCE_POWER_KILL
        {32, {{"usertype", "1"}, {"prerequisites", "50_9"}, {"guardian", "12"}, {"consular", "12"}, {"sentinel", "12"}, {"dark_recom", "6"}}},
        // FORCE_POWER_KNIGHT_VALOR
        {33, {{"usertype", "1"}, {"prerequisites", "22"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"light_recom", "20"}}},
        // FORCE_POWER_KNIGHT_SPEED
        {34, {{"usertype", "1"}, {"prerequisites", "8"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"light_recom", "17"}, {"dark_recom", "19"}}},
        // FORCE_POWER_LIGHTNING
        {35, {{"usertype", "1"}, {"prerequisites", "43"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"dark_recom", "2"}}},
        // FORCE_POWER_MASTER_VALOR
        {36, {{"usertype", "1"}, {"prerequisites", "22_33"}, {"guardian", "15"}, {"consular", "15"}, {"sentinel", "15"}, {"light_recom", "21"}}},
        // FORCE_POWER_SPEED_MASTERY
        {37, {{"usertype", "1"}, {"prerequisites", "8_34"}, {"guardian", "15"}, {"consular", "15"}, {"sentinel", "15"}, {"light_recom", "18"}, {"dark_recom", "20"}}},
        // FORCE_POWER_PLAGUE
        {38, {{"usertype", "1"}, {"prerequisites", "7_45"}, {"guardian", "12"}, {"consular", "12"}, {"sentinel", "12"}, {"dark_recom", "11"}}},
        // FORCE_POWER_RESIST_ENERGY_2
        {40, {{"usertype", "1"}, {"prerequisites", "42"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"light_recom", "23"}, {"dark_recom", "28"}}},
        // FORCE_POWER_RESIST_FORCE
        {41, {{"usertype", "1"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"light_recom", "24"}, {"dark_recom", "25"}}},
        // FORCE_POWER_RESIST_ENERGY_1
        {42, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"light_recom", "22"}, {"dark_recom", "27"}}},
        // FORCE_POWER_SHOCK
        {43, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"dark_recom", "1"}}},
        // FORCE_POWER_MASS_STASIS
        {44, {{"usertype", "1"}, {"prerequisites", "46_29"}, {"guardian", "15"}, {"consular", "15"}, {"sentinel", "15"}, {"light_recom", "3"}}},
        // FORCE_POWER_SLOW
        {45, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"dark_recom", "9"}}},
        // FORCE_POWER_STUN
        {46, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"light_recom", "1"}}},
        // FORCE_POWER_DROID_STUN
        {47, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"light_recom", "10"}}},
        // FORCE_POWER_SUPRESS_FORCE
        {48, {{"usertype", "1"}, {"guardian", "9"}, {"consular", "9"}, {"sentinel", "9"}, {"light_recom", "26"}, {"dark_recom", "23"}}},
        // FORCE_POWER_LIGHT_SABER_THROW
        {49, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"light_recom", "8"}, {"dark_recom", "21"}}},
        // FORCE_POWER_WOUND
        {50, {{"usertype", "1"}, {"guardian", "0"}, {"consular", "0"}, {"sentinel", "0"}, {"dark_recom", "4"}}},
    };
    return makeSparseTable(columns, 132, rows);
}

std::shared_ptr<Skill> makeSkill(const TwoDA &table, int row) {
    auto skill = std::make_shared<Skill>();
    skill->npcCanUse = table.getInt(row, "npccanuse") == 1;
    skill->droidCanUse = table.getInt(row, "droidcanuse") == 1;
    return skill;
}

class RecommendedTest : public Test {
protected:
    NiceMock<MockTextures> textures;
    NiceMock<MockAudioClips> audioClips;
    NiceMock<MockModels> models;
    NiceMock<MockStrings> strings;
    NiceMock<MockTwoDAs> twoDas;
    NiceMock<MockSkills> skills;
    Races races;
    std::shared_ptr<TwoDA> skillsTable {makeSkillsTable()};
    std::unique_ptr<Classes> classes;
    std::unique_ptr<Feats> feats;
    std::unique_ptr<Spells> spells;

    void SetUp() override {
        ON_CALL(twoDas, get("classes")).WillByDefault(Return(makeClassesTable()));
        ON_CALL(twoDas, get("skills")).WillByDefault(Return(skillsTable));
        ON_CALL(twoDas, get("feat")).WillByDefault(Return(makeFeatTable()));
        ON_CALL(twoDas, get("featgain")).WillByDefault(Return(makeFeatGainTable()));
        ON_CALL(twoDas, get("spells")).WillByDefault(Return(makeSpellsTable()));
        ON_CALL(twoDas, get("ranges")).WillByDefault(Return(makeTable({"primaryrange", "secondaryrange"}, {})));
        ON_CALL(twoDas, get("save")).WillByDefault(Return(makeTable({"level", "fortsave", "refsave", "willsave"}, {{"1", "0", "0", "0"}})));
        ON_CALL(twoDas, get("attack")).WillByDefault(Return(makeTable({"bab"}, {{"0"}, {"1"}, {"2"}, {"3"}})));
        ON_CALL(twoDas, get("racialtypes")).WillByDefault(Return(makeTable({"featstable"}, std::vector<std::vector<std::string>>(static_cast<int>(RacialType::Human) + 1, {""}))));
        ON_CALL(skills, get(_)).WillByDefault([this](SkillType skill) {
            return makeSkill(*skillsTable, static_cast<int>(skill));
        });

        races.init(twoDas);
        classes = std::make_unique<Classes>(strings, twoDas);
        feats = std::make_unique<Feats>(textures, strings, twoDas, races, skills);
        feats->init();
        spells = std::make_unique<Spells>(textures, audioClips, models, strings, twoDas);
        spells->init(GameID::KotOR);
    }

    CreatureClass &scoundrel() { return *classes->get(ClassType::Scoundrel); }

    /** A new Scoundrel with the class's default abilities, Int 14, and its first-level feats. */
    CreatureAttributes newScoundrel() {
        CreatureAttributes attributes(scoundrel().defaultAttributes());
        feats->addGrantedFeats(attributes, RacialType::Human, 0, kObjectTagPlayer, false, true);
        return attributes;
    }

    CreatureAttributes levelled(const CreatureAttributes &attributes) {
        CreatureAttributes copy(attributes);
        copy.addClassLevels(copy.classLevels().back().first, 1);
        return copy;
    }

    std::vector<FeatType> recommendFeats(const CreatureAttributes &attributes, int normal, int bonus) {
        std::set<FeatType> noEffectFeats;
        recommended::FeatSpending spending {
            attributes, *attributes.classLevels().back().first, normal, bonus, noEffectFeats, kObjectTagPlayer, false};
        return recommended::recommendFeats(*feats, spending);
    }

    std::map<SkillType, int> ranks(const CreatureAttributes &attributes) {
        std::map<SkillType, int> result;
        for (int skill = 0; skill < 8; ++skill) {
            int rank = attributes.getSkillRank(static_cast<SkillType>(skill));
            if (rank > 0) {
                result[static_cast<SkillType>(skill)] = rank;
            }
        }
        return result;
    }
};

} // namespace

TEST_F(RecommendedTest, should_follow_the_worked_example_of_a_scoundrel_from_level_1_to_2) {
    // Level 1, at character generation: Int 14 gives 24 points; class skills
    // cap at 4 ranks, cross-class skills at 2.
    CreatureAttributes attributes(newScoundrel());
    ASSERT_EQ(attributes.intelligence(), 14);
    int points = recommended::getSkillPointGain(scoundrel().skillPointBase(), attributes.getAbilityModifier(Ability::Intelligence), 1);
    ASSERT_EQ(points, 24);
    points = recommended::recommendSkills(attributes, points, {skills, 1, true, false});

    EXPECT_EQ(points, 0);
    EXPECT_THAT(ranks(attributes), ElementsAre(
                                       Pair(SkillType::Demolitions, 4),
                                       Pair(SkillType::Stealth, 4),
                                       Pair(SkillType::Awareness, 4),
                                       Pair(SkillType::Repair, 2),
                                       Pair(SkillType::Security, 4),
                                       Pair(SkillType::TreatInjury, 2)));
    EXPECT_TRUE(attributes.hasFeat(FeatType::SniperShot));
    auto level1Feats = recommendFeats(attributes, 1, 0);
    EXPECT_THAT(level1Feats, ElementsAre(FeatType::Dueling));
    attributes.addFeat(FeatType::Dueling);

    // Level 2: 5 points and a class cap of 5. Treat Injury and Repair are at
    // their cap, Computer Use costs the 2 points the last one cannot pay, and
    // Persuade takes it.
    points = recommended::getSkillPointGain(scoundrel().skillPointBase(), attributes.getAbilityModifier(Ability::Intelligence), 2);
    ASSERT_EQ(points, 5);
    points = recommended::recommendSkills(attributes, points, {skills, 2, true, false});

    EXPECT_EQ(points, 0);
    EXPECT_THAT(ranks(attributes), ElementsAre(
                                       Pair(SkillType::Demolitions, 5),
                                       Pair(SkillType::Stealth, 5),
                                       Pair(SkillType::Awareness, 5),
                                       Pair(SkillType::Persuade, 1),
                                       Pair(SkillType::Repair, 2),
                                       Pair(SkillType::Security, 5),
                                       Pair(SkillType::TreatInjury, 2)));
    // Dueling is held and Sniper Shot is a granted feat, so the third
    // priority, Gear Head, is chosen.
    EXPECT_THAT(recommendFeats(levelled(attributes), 1, 0), ElementsAre(FeatType::GearHead));
}

TEST_F(RecommendedTest, should_auto_level_a_scoundrel_as_its_level_up_screens_recommend) {
    CreatureAttributes attributes(newScoundrel());
    recommended::recommendSkills(attributes, 24, {skills, 1, true, false});
    attributes.addFeat(FeatType::Dueling);

    recommended::AutoLevelUp character {*feats, skills, *spells, RacialType::Human, 2000, kObjectTagPlayer, false, true, 50, {}, {}};
    CreatureAttributes chosen(recommended::chooseAutoLevel(attributes, character));

    // The new class level is left to Creature::applyLevelUp.
    EXPECT_EQ(chosen.getAggregateLevel(), 1);
    EXPECT_EQ(chosen.getSkillRank(SkillType::Stealth), 5);
    EXPECT_EQ(chosen.getSkillRank(SkillType::Persuade), 1);
    EXPECT_EQ(chosen.getSkillRank(SkillType::ComputerUse), 0);
    EXPECT_TRUE(chosen.hasFeat(FeatType::GearHead));
    EXPECT_EQ(chosen.dexterity(), 16);
}

TEST_F(RecommendedTest, should_raise_the_primary_ability_at_every_fourth_class_level_on_auto_level_up) {
    CreatureAttributes attributes(newScoundrel());
    attributes.addClassLevels(&scoundrel(), 2);

    recommended::AutoLevelUp character {*feats, skills, *spells, RacialType::Human, 6000, kObjectTagPlayer, false, true, 50, {}, {}};
    CreatureAttributes chosen(recommended::chooseAutoLevel(attributes, character));

    EXPECT_EQ(chosen.dexterity(), 17);
    EXPECT_EQ(chosen.strength(), 10);
}

TEST_F(RecommendedTest, should_fall_back_to_the_priority_zero_skill_at_the_priority_as_an_index) {
    // The test class has every skill at priorities 1 to 7 but Persuade, at
    // 0: priority 8 falls back to the eighth priority-0 skill, or the last.
    auto &testClass = *classes->get(ClassType::Minion);
    EXPECT_EQ(recommended::getRecommendedSkill(testClass, 3), SkillType::Stealth);
    EXPECT_EQ(recommended::getRecommendedSkill(testClass, 8), SkillType::Persuade);
    EXPECT_EQ(recommended::getRecommendedSkill(testClass, 0), SkillType::Persuade);

    // A blank priority is not 0: the Soldier's Persuade cell is blank, so
    // the Soldier has no skill at priority 8.
    auto &soldier = *classes->get(ClassType::Soldier);
    EXPECT_EQ(recommended::getRecommendedSkill(soldier, 1), SkillType::TreatInjury);
    EXPECT_FALSE(recommended::getRecommendedSkill(soldier, 8));
}

TEST_F(RecommendedTest, should_fall_back_to_the_priority_zero_feat_at_the_priority_as_an_index) {
    // The Scoundrel's priorities run from 1 to 9. Past them a priority picks
    // that index among the feats with priority 0 (a blank cell), in row order,
    // leaving out the feats off the class's list and the rows without a name.
    EXPECT_EQ(recommended::getRecommendedFeat(scoundrel(), 3), FeatType::GearHead);
    EXPECT_EQ(recommended::getRecommendedFeat(scoundrel(), 10), FeatType::ImplantLevel1);
    EXPECT_EQ(recommended::getRecommendedFeat(scoundrel(), 60), FeatType::MasterToughness);
}

TEST_F(RecommendedTest, should_fall_back_to_the_priority_zero_power_one_index_lower) {
    // Three Force powers with no light side priority and one with priority 1.
    NiceMock<MockSpells> fixture;
    std::vector<std::shared_ptr<Spell>> rows;
    for (int row = 0; row < 5; ++row) {
        auto spell = std::make_shared<Spell>();
        spell->type = static_cast<SpellType>(row);
        spell->userType = row == 1 ? kSpellUserTypeSpecialAbility : kSpellUserTypeForcePower;
        spell->lightRecommendation = row == 4 ? 1 : 0;
        rows.push_back(spell);
    }
    ON_CALL(fixture, get(_)).WillByDefault([&rows](SpellType type) {
        int row = static_cast<int>(type);
        return row >= 0 && row < static_cast<int>(rows.size()) ? rows[row] : nullptr;
    });

    EXPECT_EQ(recommended::getRecommendedPower(fixture, true, 1), static_cast<SpellType>(4));
    // Priority 2 falls back to index 1 of rows 0, 2 and 3 (row 1 is not a
    // Force power), where the skills and feats would take index 2.
    EXPECT_EQ(recommended::getRecommendedPower(fixture, true, 2), static_cast<SpellType>(2));
    EXPECT_EQ(recommended::getRecommendedPower(fixture, true, 3), static_cast<SpellType>(3));
    EXPECT_EQ(recommended::getRecommendedPower(fixture, true, 9), static_cast<SpellType>(3));
}

TEST_F(RecommendedTest, should_recommend_dark_side_powers_up_to_goodevil_40_and_light_side_ones_above) {
    CreatureAttributes attributes(classes->get(ClassType::JediGuardian)->defaultAttributes());
    auto &guardian = *classes->get(ClassType::JediGuardian);

    EXPECT_THAT(recommended::recommendPowers(*spells, attributes, guardian, 1, 40), ElementsAre(SpellType::Shock));
    EXPECT_THAT(recommended::recommendPowers(*spells, attributes, guardian, 1, 41), ElementsAre(SpellType::Stun));
    EXPECT_THAT(recommended::recommendPowers(*spells, attributes, guardian, 1, 0), ElementsAre(SpellType::Shock));
    EXPECT_THAT(recommended::recommendPowers(*spells, attributes, guardian, 1, 100), ElementsAre(SpellType::Stun));
}

TEST_F(RecommendedTest, should_recommend_powers_the_character_level_reaches_with_their_prerequisites) {
    auto &guardian = *classes->get(ClassType::JediGuardian);

    // A first-level Guardian: Stasis, Mass Stasis and Force Whirlwind need
    // level 9 or more and Cure (first of the two powers at priority 6) level
    // 6, so the third power is Saber Throw, at priority 8.
    CreatureAttributes jedi(guardian.defaultAttributes());
    EXPECT_THAT(recommended::recommendPowers(*spells, jedi, guardian, 3, 50),
                ElementsAre(SpellType::Stun, SpellType::ForcePush, SpellType::LightSaberThrow));

    // A Scoundrel 8 who becomes a Guardian is at character level 9: Stasis is
    // open, its prerequisite Stun chosen just before it.
    CreatureAttributes multiclass(newScoundrel());
    multiclass.addClassLevels(&scoundrel(), 7);
    multiclass.addClassLevels(&guardian, 1);
    EXPECT_THAT(recommended::recommendPowers(*spells, multiclass, guardian, 3, 50),
                ElementsAre(SpellType::Stun, SpellType::Hold, SpellType::ForcePush));

    // A known power is passed over, and none is recommended to a class that
    // is not a Jedi class.
    multiclass.addSpell(SpellType::Stun, ClassType::JediGuardian);
    EXPECT_THAT(recommended::recommendPowers(*spells, multiclass, guardian, 1, 50), ElementsAre(SpellType::Hold));
    EXPECT_THAT(recommended::recommendPowers(*spells, multiclass, scoundrel(), 1, 50), IsEmpty());
}

TEST_F(RecommendedTest, should_spend_an_npcs_points_only_on_skills_an_npc_or_droid_can_use) {
    CreatureAttributes attributes(newScoundrel());
    attributes.setSkillRank(SkillType::Stealth, 4);
    attributes.setSkillRank(SkillType::Demolitions, 4);
    attributes.setSkillRank(SkillType::Security, 4);
    attributes.setSkillRank(SkillType::Awareness, 4);
    attributes.setSkillRank(SkillType::TreatInjury, 2);
    attributes.setSkillRank(SkillType::Repair, 2);

    // An NPC Scoundrel cannot spend on Persuade, so its last point is left.
    CreatureAttributes npc(attributes);
    EXPECT_EQ(recommended::recommendSkills(npc, 5, {skills, 2, false, false}), 1);
    EXPECT_EQ(npc.getSkillRank(SkillType::Persuade), 0);
    EXPECT_EQ(npc.getSkillRank(SkillType::Stealth), 5);

    // A droid NPC passes over Stealth and Treat Injury as well, so its last
    // two points buy a rank of Computer Use.
    CreatureAttributes droid(attributes);
    EXPECT_EQ(recommended::recommendSkills(droid, 5, {skills, 2, false, true}), 0);
    EXPECT_EQ(droid.getSkillRank(SkillType::Stealth), 4);
    EXPECT_EQ(droid.getSkillRank(SkillType::Demolitions), 5);
    EXPECT_EQ(droid.getSkillRank(SkillType::ComputerUse), 1);
    EXPECT_EQ(droid.getSkillRank(SkillType::Persuade), 0);

    // The player character spends on Persuade even when it is a droid.
    CreatureAttributes player(attributes);
    EXPECT_EQ(recommended::recommendSkills(player, 5, {skills, 2, true, true}), 0);
    EXPECT_EQ(player.getSkillRank(SkillType::Persuade), 1);
}

TEST_F(RecommendedTest, should_cap_a_cross_class_skill_at_half_the_class_skill_cap) {
    // Repair is a cross-class skill for a Scoundrel: at a rank level of 4 a
    // class skill caps at 7 and Repair at 3, two points a rank.
    CreatureAttributes attributes(newScoundrel());
    for (auto skill : {SkillType::Stealth, SkillType::Demolitions, SkillType::Security, SkillType::Awareness, SkillType::Persuade}) {
        attributes.setSkillRank(skill, 7);
    }
    EXPECT_EQ(recommended::recommendSkills(attributes, 7, {skills, 4, true, false}), 1);
    EXPECT_EQ(attributes.getSkillRank(SkillType::TreatInjury), 3);
    EXPECT_EQ(attributes.getSkillRank(SkillType::Repair), 0);

    // At a rank level of 5 the cap is (5 + 3) / 2 = 4.
    CreatureAttributes higher(newScoundrel());
    EXPECT_EQ(recommended::recommendSkills(higher, 48, {skills, 5, true, false}), 0);
    EXPECT_EQ(higher.getSkillRank(SkillType::TreatInjury), 4);
    EXPECT_EQ(higher.getSkillRank(SkillType::Repair), 4);
    EXPECT_EQ(higher.getSkillRank(SkillType::Stealth), 8);
}

TEST(RecommendedSkillPoints, should_quadruple_the_first_level_points_with_the_whole_modifier) {
    EXPECT_EQ(recommended::getSkillPointGain(8, 2, 1), 24);
    EXPECT_EQ(recommended::getSkillPointGain(2, 0, 1), 4);
    EXPECT_EQ(recommended::getSkillPointGain(2, -2, 1), 4);
    EXPECT_EQ(recommended::getSkillPointGain(8, 2, 2), 5);
    EXPECT_EQ(recommended::getSkillPointGain(8, 1, 2), 4);
    EXPECT_EQ(recommended::getSkillPointGain(2, -1, 3), 1);
    EXPECT_EQ(recommended::getSkillPointGain(2, -4, 3), 1);
}
