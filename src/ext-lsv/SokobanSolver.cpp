using namespace std;
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
#include <unordered_set>
#include <cmath>
#include <utility>
#include <queue>
#include "SokobanSolver.h"
#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "set"
#include "queue"
#include "algorithm"
#include "sat/cnf/cnf.h"
#include "stdint.h"

void SokobanSolver::CnfWriter(sat_solver *pSat)
{
    // Ensure the SAT solver is aware of the maximum variable index
    for (const auto &cl : clauses)
    {
        lit *newClause = new lit[cl->clause.size()];
        int i = 0;
        for (const auto &lit : cl->clause)
        {
            if (lit.x < 0)
                newClause[i] = Abc_Var2Lit(abs(lit.x), 1);
            else
                newClause[i] = Abc_Var2Lit(lit.x, 0);
            i++;
        }
        // cout << "Adding clause: ";
        /*for (int j = 0; j < cl->clause.size(); j++)
        {
            cout << newClause[j] << " ";
        }
        cout << endl;*/
        if (!sat_solver_addclause(pSat, newClause, newClause + cl->clause.size()))
            cerr << "Failed to add clause to SAT solver" << endl;
        delete[] newClause;
    }
    return;
}
void SokobanSolver::debugger(const string &filename)
{
    ofstream outFile;
    outFile.open(filename);
    if (!outFile.is_open())
    {
        cout << "Error in creating file!" << endl;
        return;
    }
    cout << "Start Writing to debug file..." << endl;
    for (const auto &cl : clauses)
    {
        for (const auto &lit : cl->clause)
        {
            outFile << lit.x << "(" << lit.get_x() << ", " << lit.get_y() << ", " << lit.get_p() << ", " << lit.get_t() << ") ";
        }
        outFile << endl;
    }
    outFile << endl;
    outFile << "Literals Info: " << endl;
    outFile << "Player Literals: " << endl
            << "-----------------------------" << endl;
    for (auto it = playerLitManager.begin(); it != playerLitManager.end(); it++)
    {
        outFile << it->second.second.x << " " << it->second.second.get_x() << " " << it->second.second.get_y() << " " << it->second.second.get_p() << " " << it->second.second.get_t() << endl;
    }
    outFile << "Box Literals: " << endl
            << "-----------------------------" << endl;
    for (auto it = boxLitManager.begin(); it != boxLitManager.end(); it++)
    {
        outFile << it->second.second.x << " " << it->second.second.get_x() << " " << it->second.second.get_y() << " " << it->second.second.get_p() << " " << it->second.second.get_t() << endl;
    }
    outFile << "Player initial positions: " << endl;
    for (const auto &pos : mapInfo["Players"])
    {
        outFile << pos.first << " " << pos.second << endl;
    }
    outFile << "Box initial positions: " << endl;
    for (const auto &pos : mapInfo["Boxes"])
    {
        outFile << pos.first << " " << pos.second << endl;
    }
    outFile << "Lit Dictionary: " << endl;
    for (auto it = LitDictionary.begin(); it != LitDictionary.end(); it++)
    {
        outFile << it->first << ": " << it->second.get_x() << " " << it->second.get_y() << " " << it->second.get_p() << " " << it->second.get_t() << " " << it->second.get_identity() << endl;
    }
    outFile.close();
    cout << "Done" << endl;
    return;
}
void Clause::AddLit(const Lit &p)
{
    this->clause.push_back(p);
}
SokobanSolver::SokobanSolver()
{
    mapInfo["Players"] = {};
    mapInfo["Walls"] = {};
    mapInfo["Boxes"] = {};
    mapInfo["Targets"] = {};
    mapInfo["Walkable"] = {};
    this->LitIndex = 1;
};
void SokobanSolver::loadMap(const string &filename)
{
    // define input format
    const char playerSymbol = '@';
    const char wallSymbol = '#';
    const char boxSymbol = '$';
    const char targetSymbol = '.';
    const char walkableSymbol = ' ';
    const char box_on_targetSymbol = '*';
    const char player_on_targetSymbol = '+';

    ifstream inFile(filename);
    if (!inFile.is_open())
    {
        cerr << "Error: Unable to open file " << filename << endl;
        return;
    }
    string line;
    int row = 0;
    int column = 0;
    while (getline(inFile, line))
    {
        for (int col = 0; col < line.size(); col++)
        {
            char c = line[col];
            switch (c)
            {
            case playerSymbol:
                mapInfo["Players"].push_back(make_pair(row, col)); // player index in map info
                break;
            case wallSymbol:
                mapInfo["Walls"].push_back(make_pair(row, col));
                break;
            case boxSymbol:
                mapInfo["Boxes"].push_back(make_pair(row, col)); // box index in map info
                break;
            case targetSymbol:
                mapInfo["Targets"].push_back(make_pair(row, col));
                break;
            case box_on_targetSymbol:
                mapInfo["Boxes"].push_back(make_pair(row, col)); // box index in map info
                mapInfo["Targets"].push_back(make_pair(row, col));
                break;
            case player_on_targetSymbol:
                mapInfo["Players"].push_back(make_pair(row, col)); // player index in map info
                mapInfo["Targets"].push_back(make_pair(row, col));
                break;
            default:
                break;
            }
            if (c != wallSymbol)
                mapInfo["Walkable"].push_back(make_pair(row, col));
            // column = line.size();
        }
        column = max(column, (int)line.size());
        row++;
    }
    this->mapSize = make_pair(row, column);
    this->playerNum = mapInfo["Players"].size();
    this->boxNum = mapInfo["Boxes"].size();
    // cout << "Map Dimension: " << mapSize.first << ", " << mapSize.second << endl;
    // cout << "PlayerNum: " << playerNum << endl;
    // cout << "BoxNum: " << boxNum << endl;
    findDeadLockBoxPos();
    if (verbose)
        cout << "Time steps: " << stepLimit << endl;
}
void SokobanSolver::setStepLimit(int limit)
{
    this->stepLimit = limit;
}
void SokobanSolver::AddClause(Clause *newClause)
{
    this->clauses.push_back(newClause);
}
Lit SokobanSolver::AddPlayerLiteral(int row, int col, int player, int t)
{
    string key = createKey(row, col, player, t);
    if (playerLitManager.find(key) == playerLitManager.end()) // literal not instantiated yet
    {
        Lit newLit(row, col, player, t, LitIndex, 1);
        playerLitManager[key] = {LitIndex, newLit};
        LitDictionary[LitIndex] = newLit;
        this->LitIndex++;
    }
    return playerLitManager[key].second;
}
Lit SokobanSolver::AddBoxLiteral(int row, int col, int box, int t)
{
    string key = createKey(row, col, box, t);
    if (boxLitManager.find(key) == boxLitManager.end()) // literal not instantiated yet
    {
        Lit newLit(row, col, box, t, LitIndex, 0);
        boxLitManager[key] = {LitIndex, newLit};
        LitDictionary[LitIndex] = newLit;
        this->LitIndex++;
    }
    return boxLitManager[key].second;
}
vector<vector<Lit>> cartesianProduct(const vector<set<Lit>> &sets)
{
    vector<vector<Lit>> result = {{}};
    for (const auto &set : sets)
    {
        if (set.empty())
            continue;
        vector<vector<Lit>> temp;
        for (const auto &combination : result)
        {
            for (const auto &lit : set)
            {
                vector<Lit> newCombination = combination;
                newCombination.push_back(lit);
                temp.push_back(move(newCombination)); // Use move to avoid unnecessary copying
            }
        }

        result = move(temp); // Efficiently transfer ownership of the temporary result
    }

    return result;
}
void SokobanSolver::PlayerMovementConstraints()
{
    // cout << "Adding player movement constraints..." << endl;
    for (int t = 0; t < stepLimit; t++)
    {
        for (int player = 0; player < playerNum; player++)
        {
            for (auto walkable_coord : mapInfo["Walkable"])
            {
                int row = walkable_coord.first;
                int col = walkable_coord.second;
                // move up
                Clause *clause = new Clause();
                clause->AddLit(~(AddPlayerLiteral(row, col, player, t)));
                clause->AddLit(AddPlayerLiteral(row, col, player, t + 1));
                if (row > 0 && notWall(row - 1, col)) // row-1 >= 0
                    clause->AddLit(AddPlayerLiteral(row - 1, col, player, t + 1));
                // move down
                if (row < mapSize.first - 1 && notWall(row + 1, col))
                    clause->AddLit(AddPlayerLiteral(row + 1, col, player, t + 1));
                // move left
                if (col > 0 && notWall(row, col - 1))
                    clause->AddLit(AddPlayerLiteral(row, col - 1, player, t + 1));
                // move right
                if (col < mapSize.second - 1 && notWall(row, col + 1))
                    clause->AddLit(AddPlayerLiteral(row, col + 1, player, t + 1));
                AddClause(clause);
            }
        }
    }

    for (int t = 1; t <= stepLimit; t++)
    {
        for (int player = 0; player < playerNum; player++)
        {
            for (auto walkable_coord : mapInfo["Walkable"])
            {
                int row = walkable_coord.first;
                int col = walkable_coord.second;
                Clause *clause = new Clause();
                clause->AddLit(~(AddPlayerLiteral(row, col, player, t)));
                clause->AddLit(AddPlayerLiteral(row, col, player, t - 1));
                if (row > 0 && notWall(row - 1, col))
                    clause->AddLit(AddPlayerLiteral(row - 1, col, player, t - 1));
                if (row < mapSize.first - 1 && notWall(row + 1, col))
                    clause->AddLit(AddPlayerLiteral(row + 1, col, player, t - 1));
                if (col > 0 && notWall(row, col - 1))
                    clause->AddLit(AddPlayerLiteral(row, col - 1, player, t - 1));
                if (col < mapSize.second - 1 && notWall(row, col + 1))
                    clause->AddLit(AddPlayerLiteral(row, col + 1, player, t - 1));
                AddClause(clause);
            }
        }
    }
}

void SokobanSolver::BoxPushMovementConstraints()
{
    // cout << "Adding box push movement constraints..." << endl;
    for (auto walkable_coord : mapInfo["Walkable"])
    {
        int row = walkable_coord.first;
        int col = walkable_coord.second;
        if (isDeadLockBoxPos(row, col))
            continue;
        for (int box = 0; box < boxNum; box++)
        {
            for (int t = 1; t <= stepLimit; t++) // all time steps except first state t = 0
            {
                // initialize the sets to be taken cartesian products, that is, get all the variables appeared in the constraint
                // ->
                vector<set<Lit>> sets_push;
                for (int player = 0; player < playerNum; player++)
                {
                    // Ensure there are no obstacles in the push path
                    // push up
                    if (row - 2 >= 0 && notWall(row - 1, col) && !isDeadLockBoxPos(row - 1, col) && notWall(row - 2, col))
                    {
                        sets_push.push_back({AddBoxLiteral(row - 1, col, box, t - 1), AddPlayerLiteral(row - 2, col, player, t - 1), AddPlayerLiteral(row - 1, col, player, t)});
                    }
                    if (row + 2 < mapSize.first && notWall(row + 1, col) && !isDeadLockBoxPos(row + 1, col) && notWall(row + 2, col))
                    {
                        sets_push.push_back({AddBoxLiteral(row + 1, col, box, t - 1), AddPlayerLiteral(row + 2, col, player, t - 1), AddPlayerLiteral(row + 1, col, player, t)});
                    }
                    if (col + 2 < mapSize.second && notWall(row, col + 1) && !isDeadLockBoxPos(row, col + 1) && notWall(row, col + 2))
                    {
                        sets_push.push_back({AddBoxLiteral(row, col + 1, box, t - 1), AddPlayerLiteral(row, col + 2, player, t - 1), AddPlayerLiteral(row, col + 1, player, t)});
                    }
                    if (col - 2 >= 0 && notWall(row, col - 1) && !isDeadLockBoxPos(row, col - 1) && notWall(row, col - 2))
                    {
                        sets_push.push_back({AddBoxLiteral(row, col - 1, box, t - 1), AddPlayerLiteral(row, col - 2, player, t - 1), AddPlayerLiteral(row, col - 1, player, t)});
                    }
                }
                if (sets_push.empty())
                    continue;
                // execute cartesian products
                vector<vector<Lit>> Cartesian_push = cartesianProduct(sets_push);

                // iterate through the cartesian product {{a, b}, {c, d, e}, ...}
                // some sets might be {{}} since no push backs before
                for (const auto &set : Cartesian_push)
                {
                    Clause *newClause = new Clause;
                    newClause->AddLit(~(AddBoxLiteral(row, col, box, t)));
                    newClause->AddLit(AddBoxLiteral(row, col, box, t - 1));
                    if (!set.empty())
                    {
                        for (const auto &lit : set)
                            newClause->AddLit(lit);
                    }
                    AddClause(newClause);
                }
            }
        }
    }
}

void SokobanSolver::DebugConstraints()
{
    for (auto walkable_coord : mapInfo["Walkable"])
    {
        int row = walkable_coord.first;
        int col = walkable_coord.second;
        if (isDeadLockBoxPos(row, col))
            continue;
        for (int box = 0; box < boxNum; box++)
        {
            for (int t = 1; t <= stepLimit; t++)
            {
                vector<pair<int, int>> dir = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                Clause *newClause = new Clause;
                newClause->AddLit(~(AddBoxLiteral(row, col, box, t)));
                newClause->AddLit(AddBoxLiteral(row, col, box, t - 1));
                for (const auto &dir_pair : dir)
                {
                    int row_dir = dir_pair.first;
                    int col_dir = dir_pair.second;
                    if (row + row_dir >= 0 && row + row_dir < mapSize.first && col + col_dir >= 0 && col + col_dir < mapSize.second && !isDeadLockBoxPos(row + row_dir, col + col_dir) && notWall(row + row_dir, col + col_dir))
                        newClause->AddLit(AddBoxLiteral(row + row_dir, col + col_dir, box, t - 1));
                }
                // cout << "Adding Debug Constraint for Box " << box << " at (" << row << ", " << col << ") at time " << t << endl;
                AddClause(newClause);
            }
        }
    }
}

void SokobanSolver::PlayerSinglePlacementConstraints()
{
    vector<pair<int, int>> validPositions;

    // Collect all valid positions
    for (auto walkable_coord : mapInfo["Walkable"])
    {
        int row = walkable_coord.first;
        int col = walkable_coord.second;
        validPositions.push_back({row, col});
    }
    // Create clauses to ensure no two players occupy the same position at the same time
    for (int player = 0; player < playerNum; player++)
    {
        for (int t = 1; t <= stepLimit; t++)
        {
            for (size_t i = 0; i < validPositions.size(); i++)
            {
                for (size_t j = i + 1; j < validPositions.size(); j++)
                {
                    Clause *newClause = new Clause;
                    newClause->AddLit(~(AddPlayerLiteral(validPositions[i].first, validPositions[i].second, player, t)));
                    newClause->AddLit(~(AddPlayerLiteral(validPositions[j].first, validPositions[j].second, player, t)));
                    AddClause(newClause);
                }
            }
        }
    }
}
void SokobanSolver::BoxSinglePlacementConstraints()
{
    // cout << "Adding box single placement constraints..." << endl;

    vector<pair<int, int>> validPositions;

    // Collect all valid positions
    for (auto walkable_coord : mapInfo["Walkable"])
    {
        int row = walkable_coord.first;
        int col = walkable_coord.second;
        if (isDeadLockBoxPos(row, col))
            continue;
        validPositions.push_back({row, col});
    }

    // Create clauses to ensure no two players occupy the same position at the same time
    for (int box = 0; box < boxNum; box++)
    {
        for (int t = 1; t <= stepLimit; t++)
        {
            for (size_t i = 0; i < validPositions.size(); i++)
            {
                for (size_t j = i + 1; j < validPositions.size(); j++)
                {
                    Clause *newClause = new Clause;
                    newClause->AddLit(~(AddBoxLiteral(validPositions[i].first, validPositions[i].second, box, t)));
                    newClause->AddLit(~(AddBoxLiteral(validPositions[j].first, validPositions[j].second, box, t)));
                    AddClause(newClause);
                }
            }
        }
    }
}

void SokobanSolver::BoxCollisionConstraints() // should be on different positions at all time steps
{
    // cout << "Adding box collision constraints..." << endl;
    for (int box1 = 0; box1 < boxNum - 1; box1++)
    {
        for (int box2 = box1 + 1; box2 < boxNum; box2++)
        {
            for (auto walkable_coord : mapInfo["Walkable"])
            {
                int row = walkable_coord.first;
                int col = walkable_coord.second;
                if (isDeadLockBoxPos(row, col))
                    continue;
                for (int t = 1; t <= stepLimit; t++)
                {
                    Clause *newClause = new Clause;
                    newClause->AddLit(~(AddBoxLiteral(row, col, box1, t)));
                    newClause->AddLit(~(AddBoxLiteral(row, col, box2, t)));
                    AddClause(newClause);
                }
            }
        }
    }
}

void SokobanSolver::BoxAndPlayerCollisionConstraints()
{
    // cout << "Adding box and player collision constraints..." << endl;
    for (int player = 0; player < playerNum; player++)
    {
        for (int box = 0; box < boxNum; box++)
        {
            for (auto walkable_coord : mapInfo["Walkable"])
            {
                int row = walkable_coord.first;
                int col = walkable_coord.second;
                if (isDeadLockBoxPos(row, col))
                    continue;
                for (int t = 1; t <= stepLimit; t++)
                {
                    Clause *newClause = new Clause;
                    newClause->AddLit(~(AddBoxLiteral(row, col, box, t)));
                    newClause->AddLit(~(AddPlayerLiteral(row, col, player, t)));
                    AddClause(newClause);
                }
            }
        }
    }
}

/*
This part is for Multi-Agent
Sokoban Puzzle Solving.
*/
void SokobanSolver::PlayerCollisionConstraints()
{
    // cout << "Adding player collision constraints..." << endl;
    for (int player1 = 0; player1 < playerNum - 1; player1++)
    {
        for (int player2 = player1 + 1; player2 < playerNum; player2++)
        {
            for (auto walkable_coord : mapInfo["Walkable"])
            {
                int row = walkable_coord.first;
                int col = walkable_coord.second;
                for (int t = 1; t <= stepLimit; t++)
                {
                    Clause *newClause = new Clause;
                    newClause->AddLit(~(AddPlayerLiteral(row, col, player1, t)));
                    newClause->AddLit(~(AddPlayerLiteral(row, col, player2, t)));
                    AddClause(newClause);
                }
            }
        }
    }
}
void SokobanSolver::PlayerHeadOnConstraints() // vertical
{
    // cout << "Adding player head on constraints..." << endl;
    for (int player1 = 0; player1 < playerNum; player1++)
    {
        for (int player2 = player1 + 1; player2 < playerNum; player2++)
        {
            for (auto walkable_coord : mapInfo["Walkable"])
            {
                int row = walkable_coord.first;
                int col = walkable_coord.second;
                for (int t = 1; t < stepLimit; t++)
                {
                    // consider boundary conditions
                    if (notWall(row, col + 1))
                    {
                        Clause *newClause1 = new Clause;
                        newClause1->AddLit(~(AddPlayerLiteral(row, col, player1, t)));
                        newClause1->AddLit(~(AddPlayerLiteral(row, col + 1, player1, t + 1)));
                        newClause1->AddLit(~(AddPlayerLiteral(row, col + 1, player2, t)));
                        newClause1->AddLit(~(AddPlayerLiteral(row, col, player2, t + 1)));
                        AddClause(newClause1);

                        Clause *newClause2 = new Clause;
                        newClause2->AddLit(~(AddPlayerLiteral(row, col, player2, t)));
                        newClause2->AddLit(~(AddPlayerLiteral(row, col + 1, player2, t + 1)));
                        newClause2->AddLit(~(AddPlayerLiteral(row, col + 1, player1, t)));
                        newClause2->AddLit(~(AddPlayerLiteral(row, col, player1, t + 1)));
                        AddClause(newClause2);
                    }
                }
            }
        }
    }
    for (int player1 = 0; player1 < playerNum; player1++)
    {
        for (int player2 = player1 + 1; player2 < playerNum; player2++)
        {
            for (auto walkable_coord : mapInfo["Walkable"])
            {
                int row = walkable_coord.first;
                int col = walkable_coord.second;
                for (int t = 1; t < stepLimit; t++)
                {
                    if (notWall(row + 1, col))
                    {
                        // consider boundary conditions
                        Clause *newClause1 = new Clause;
                        newClause1->AddLit(~(AddPlayerLiteral(row, col, player1, t)));
                        newClause1->AddLit(~(AddPlayerLiteral(row + 1, col, player1, t + 1)));
                        newClause1->AddLit(~(AddPlayerLiteral(row + 1, col, player2, t)));
                        newClause1->AddLit(~(AddPlayerLiteral(row, col, player2, t + 1)));
                        AddClause(newClause1);

                        Clause *newClause2 = new Clause;
                        newClause2->AddLit(~(AddPlayerLiteral(row, col, player2, t)));
                        newClause2->AddLit(~(AddPlayerLiteral(row + 1, col, player2, t + 1)));
                        newClause2->AddLit(~(AddPlayerLiteral(row + 1, col, player1, t)));
                        newClause2->AddLit(~(AddPlayerLiteral(row, col, player1, t + 1)));
                        AddClause(newClause2);
                    }
                }
            }
        }
    }
}
/*
Multi-Agent
Sokoban Solving
*/

void SokobanSolver::InitState()
{
    // cout << "Adding initial state..." << endl;
    // can be optimized
    // at state t = 0 and for all players
    for (int player = 0; player < playerNum; player++)
    {
        Clause *PlayerClause = new Clause();
        int playerPos_row = mapInfo["Players"][player].first;
        int playerPos_col = mapInfo["Players"][player].second;
        PlayerClause->AddLit(AddPlayerLiteral(playerPos_row, playerPos_col, player, 0));
        AddClause(PlayerClause);

        for (int row = 0; row < mapSize.first; row++)
        {
            for (int col = 0; col < mapSize.second; col++)
            {
                if (!(row == playerPos_row && col == playerPos_col) && notWall(row, col))
                {
                    Clause *NotPlayerClause = new Clause();
                    NotPlayerClause->AddLit(~(AddPlayerLiteral(row, col, player, 0)));
                    AddClause(NotPlayerClause);
                }
            }
        }
    }
    for (int box = 0; box < boxNum; box++)
    {
        Clause *BoxClause = new Clause();
        int boxPos_row = mapInfo["Boxes"][box].first;
        int boxPos_col = mapInfo["Boxes"][box].second;
        BoxClause->AddLit(AddBoxLiteral(boxPos_row, boxPos_col, box, 0));
        AddClause(BoxClause);
        // cout << box << endl;
        for (int row = 0; row < mapSize.first; row++)
        {
            for (int col = 0; col < mapSize.second; col++)
            {
                if (isDeadLockBoxPos(row, col))
                    continue;
                if (!(row == boxPos_row && col == boxPos_col) && (notWall(row, col)))
                {
                    Clause *NotBoxClause = new Clause();
                    NotBoxClause->AddLit(~(AddBoxLiteral(row, col, box, 0)));
                    AddClause(NotBoxClause);
                }
            }
        }
    }
}
void SokobanSolver::SolvedState()
{
    // cout << "Adding solved state..." << endl;
    // box on target coordinates at t = stepLimit
    for (const auto &target : mapInfo["Targets"])
    { //(string) -> vector<pair<int, int>>
        Clause *newClause = new Clause;
        for (int box = 0; box < boxNum; box++)
        {
            newClause->AddLit(AddBoxLiteral(target.first, target.second, box, stepLimit));
        }
        AddClause(newClause);
    }
}
/*///////////////////////////////////
Experimental Constraints
*/
///////////////////////////////////
void SokobanSolver::LearntConstraints() // slows down the process!
{
    vector<set<Lit>> learnt_set;
    // try previous step
    for (const auto &target : mapInfo["Targets"])
    {
        set<Lit> tmp = {};
        for (int box = 0; box < boxNum; box++)
            tmp.insert(AddBoxLiteral(target.first, target.second, box, stepLimit - 1)); // last step limit
        learnt_set.push_back(tmp);
    }
    vector<vector<Lit>> Cartesian_learnt = cartesianProduct(learnt_set);
    for (const auto &set : Cartesian_learnt)
    {
        if (!set.empty())
        {
            Clause *newClause = new Clause;
            for (const auto &lit : set)
                newClause->AddLit(~lit);
            AddClause(newClause);
        }
    }
}

void SokobanSolver::bfs(vector<vector<char>> &underlyingTiles, vector<vector<bool>> &visited, vector<pair<int, int>> &group, int i, int j)
{
    visited[i][j] = true;
    group.push_back(make_pair(i, j));

    // up (need the down tile to be empty)
    if (i > 0 && !isWall(i - 1, j) && !visited[i - 1][j] && !isWall(i + 1, j))
    {
        bfs(underlyingTiles, visited, group, i - 1, j);
    }
    // down (need the up tile to be empty)
    if (i < mapSize.first - 1 && !isWall(i + 1, j) && !visited[i + 1][j] && !isWall(i - 1, j))
    {
        bfs(underlyingTiles, visited, group, i + 1, j);
    }
    // left (need the right tile to be empty)
    if (j > 0 && !isWall(i, j - 1) && !visited[i][j - 1] && !isWall(i, j + 1))
    {
        bfs(underlyingTiles, visited, group, i, j - 1);
    }
    // right (need the left tile to be empty)
    if (j < mapSize.second - 1 && !isWall(i, j + 1) && !visited[i][j + 1] && !isWall(i, j - 1))
    {
        bfs(underlyingTiles, visited, group, i, j + 1);
    }
    return;
}

void SokobanSolver::findDeadLockBoxPos()
{
    vector<vector<char>> underlyingTiles(mapSize.first, vector<char>(mapSize.second, ' '));
    for (int i = 0; i < mapSize.first; i++)
    {
        for (int j = 0; j < mapSize.second; j++)
        {
            if (isWall(i, j))
                underlyingTiles[i][j] = 'W';
            if (isTarget(i, j))
                underlyingTiles[i][j] = 'T';
        }
    }
    int deadGroupCount = 0;
    deadLockBoxPos.clear();
    // BFS to find all the dead lock box positions
    for (int i = 0; i < mapSize.first; i++)
    {
        for (int j = 0; j < mapSize.second; j++)
        {
            if (underlyingTiles[i][j] != ' ')
                continue;
            // BFS
            vector<vector<bool>> visited(mapSize.first, vector<bool>(mapSize.second, false));
            vector<pair<int, int>> group;
            group.push_back(make_pair(i, j));
            bfs(underlyingTiles, visited, group, i, j);

            // if there is a target in the group, then it is not a dead lock
            bool isDeadLock = true;
            for (auto pos : group)
            {
                if (underlyingTiles[pos.first][pos.second] == 'T')
                {
                    isDeadLock = false;
                    break;
                }
            }
            if (isDeadLock)
            {
                for (auto pos : group)
                {
                    if (underlyingTiles[pos.first][pos.second] == ' ')
                    {
                        underlyingTiles[pos.first][pos.second] = char(deadGroupCount + '0');
                        deadLockBoxPos.push_back(pos);
                    }
                }
                deadGroupCount++;
            }
        }
    }

    if (verbose)
        cout << "Num of dead lock box positions: " << deadLockBoxPos.size() << endl;
    vector<vector<char>> deadLockBoxPosStr(mapSize.first, vector<char>(mapSize.second, ' '));
    // deadLockBoxPosMap
    deadLockBoxPosMap = vector<vector<bool>>(mapSize.first, vector<bool>(mapSize.second, false));
    for (const auto &d : deadLockBoxPos)
    {
        int i = d.first;
        int j = d.second;
        deadLockBoxPosMap[i][j] = true;
    }
}

void SokobanSolver::ExistenceConstraints()
{
    // cout << "Adding existence constraints..." << endl;
    for (int t = 1; t < stepLimit; t++)
    {
        for (int box = 0; box < boxNum; box++)
        {
            Clause *newClause = new Clause;
            for (auto walkable_coord : mapInfo["Walkable"])
            {
                int row = walkable_coord.first;
                int col = walkable_coord.second;
                if (isDeadLockBoxPos(row, col))
                    continue;
                newClause->AddLit(AddBoxLiteral(row, col, box, t));
            }
            AddClause(newClause);
        }
    }
}

bool SokobanSolver::helper(vector<vector<int>> &tunnel_map, vector<vector<bool>> &visited, int row, int col, int index, int type, vector<pair<int, int>> &t)
{ // dfs for tunnels
    visited[row][col] = true;
    // base case
    if (type == 0) // horizontal
    {
        if ((notWall(row - 1, col) || notWall(row + 1, col)))
        {
            if (index >= 2)
            {
                t.push_back({row, col - 1});
                return true;
            }
            else
                return false;
        }
        return helper(tunnel_map, visited, row, col + 1, index + 1, 0, t);
    }
    else
    { // vertical
        if (notWall(row, col + 1) || notWall(row, col - 1))
            if (index >= 2)
            {
                t.push_back({row - 1, col});
                return true;
            }
            else
                return false;
        return helper(tunnel_map, visited, row + 1, col, index + 1, 1, t);
    }
    tunnel_map[row][col] = index + 1;
}
void SokobanSolver::TunnelIdentifying()
{
    // find tunnel in the map
    // create constraints for B in tunnel and P at the right place
    // Find tunnels in the map
    vector<vector<bool>> visited(mapSize.first, vector<bool>(mapSize.second, false));
    vector<vector<int>> tunnel_map(mapSize.first, vector<int>(mapSize.second, 0));

    for (int row = 1; row < mapSize.first - 1; row++)
    {
        for (int col = 1; col < mapSize.second - 1; col++)
        {
            if (!isWalkable(row, col) || visited[row][col])
                continue;
            // Check for horizontal tunnel
            if (isWall(row - 1, col) && isWall(row + 1, col))
            {
                vector<pair<int, int>> t;
                // Check if left and right are walkable
                if ((col > 0 && isWalkable(row, col - 1)) &&
                    (col < mapSize.second - 1 && isWalkable(row, col + 1)))
                {
                    tunnel_map[row][col] = 1; // tunnel entry
                    if (helper(tunnel_map, visited, row, col + 1, 1, 0, t))
                        t.push_back({row, col});
                }
                if (t.size() != 0)
                    tunnels.insert(t);
            }
            // Check for vertical tunnel
            if (isWall(row, col - 1) && isWall(row, col + 1))
            {
                vector<pair<int, int>> t;
                // Check if up and down are walkable
                if ((row > 0 && isWalkable(row - 1, col)) &&
                    (row < mapSize.first - 1 && isWalkable(row + 1, col)))
                {
                    tunnel_map[row][col] = 1; // tunnel entry
                    if (helper(tunnel_map, visited, row + 1, col, 1, 1, t))
                        t.push_back({row, col});
                }
                if (t.size() != 0)
                    tunnels.insert(t);
            }
        }
    }
    // Debug output
    if (verbose)
    {
        cout << "Identified Tunnels:" << endl;
        for (const auto &tunnel : tunnels)
        {
            cout << "Tunnel: ";
            for (const auto &pos : tunnel)
            {
                cout << "(" << pos.first << ", " << pos.second << ") ";
            }
            cout << endl;
        }
        for (int i = 0; i < mapSize.first; i++)
        {
            for (int j = 0; j < mapSize.second; j++)
            {
                if (isWall(i, j))
                    cout << "W";
                else if (tunnel_map[i][j] != ' ')
                    cout << tunnel_map[i][j];
                else
                    cout << " ";
            }
            cout << endl;
        }
    }
}
void SokobanSolver::tunnelMacro()
{
    // player only
    cout << "Size of tunnels: " << tunnels.size() << endl;

    if (tunnels.empty())
    {
        cout << "ah ha i am empty" << endl;
        return;
    }
    for (const auto &tunnel : tunnels)
    {
        // if player enters the tunnel, move through it
        //  P(row, col-1, p, t-1) ^ P(row, col, p, t) -> P(row, col+1, p, t+1)
        //  four directions
        //  identify horizontal or vertical tunnel
        if (tunnel[1].first == tunnel[0].first)
        { // horizontal tunnel, same row
            int tunnel_length = tunnel[0].second - tunnel[1].second + 1;
            // 2 entries, opposite directions
            for (int t = 1; t <= stepLimit; t++)
            {
                // note: Tunnel: (1, 5) (1, 3) tunnel stored this way
                // enter from left
                for (int curr = 0; curr < tunnel_length; curr++)
                {
                    Clause *newClause1 = new Clause;
                    newClause1->AddLit(~(AddPlayerLiteral(tunnel[0].first, tunnel[1].second + curr - 1, 0, t - 1)));
                    newClause1->AddLit(~(AddPlayerLiteral(tunnel[0].first, tunnel[1].second + curr, 0, t)));
                    newClause1->AddLit(AddPlayerLiteral(tunnel[0].first, tunnel[1].second + curr + 1, 0, t + 1));
                    AddClause(newClause1);
                }
                // enter from right
                for (int curr = 0; curr < tunnel_length; curr++)
                {
                    Clause *newClause2 = new Clause;
                    newClause2->AddLit(~(AddPlayerLiteral(tunnel[0].first, tunnel[0].second - curr + 1, 0, t - 1)));
                    newClause2->AddLit(~(AddPlayerLiteral(tunnel[0].first, tunnel[0].second - curr, 0, t)));
                    newClause2->AddLit(AddPlayerLiteral(tunnel[0].first, tunnel[0].second - curr - 1, 0, t + 1));
                    AddClause(newClause2);
                }
            }
        }
        if (tunnel[1].second == tunnel[0].second) // vertical tunnel, same column
        {
            int tunnel_length = tunnel[0].second - tunnel[1].second + 1;
            for (int t = 1; t <= stepLimit; t++)
            {
                // enter from above
                for (int curr = 0; curr < tunnel_length; curr++)
                {
                    Clause *newClause1 = new Clause;
                    newClause1->AddLit(~(AddPlayerLiteral(tunnel[1].first + curr - 1, tunnel[0].second, 0, t - 1)));
                    newClause1->AddLit(~(AddPlayerLiteral(tunnel[1].first + curr, tunnel[0].second, 0, t)));
                    newClause1->AddLit(AddPlayerLiteral(tunnel[1].first + curr + 1, tunnel[0].second, 0, t + 1));
                    AddClause(newClause1);
                }
                // enter from bottom
                for (int curr = 0; curr < tunnel_length; curr++)
                {
                    Clause *newClause2 = new Clause;
                    newClause2->AddLit(~(AddPlayerLiteral(tunnel[0].first - curr + 1, tunnel[0].second, 0, t - 1)));
                    newClause2->AddLit(~(AddPlayerLiteral(tunnel[0].first - curr, tunnel[0].second, 0, t)));
                    newClause2->AddLit(AddPlayerLiteral(tunnel[0].first - curr - 1, tunnel[0].second, 0, t + 1));
                    AddClause(newClause2);
                }
            }
        }
    }
}
void SokobanSolver::AllConstraints()
{
    InitState();
    SolvedState();
    TunnelIdentifying();
    PlayerMovementConstraints();
    BoxPushMovementConstraints();
    // PlayerHeadOnConstraints(); // MA
    PlayerSinglePlacementConstraints();
    BoxSinglePlacementConstraints();
    // PlayerCollisionConstraints(); // MA
    BoxCollisionConstraints();
    BoxAndPlayerCollisionConstraints();
    ExistenceConstraints();
    DebugConstraints();
    // tunnelMacro();

    // ObstacleConstraints();
    // LearntConstraints();
}