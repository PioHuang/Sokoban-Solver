#ifndef SOKOBAN_SOLVER_H
#define SOKOBAN_SOLVER_H
using namespace std;
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "set"
#include "queue"
#include "algorithm"
#include "sat/cnf/cnf.h"
#include "stdint.h"

class Lit
{
public:
    friend Lit operator~(const Lit &p);
    int x; // variable index number
    Lit() {}
    Lit(int row, int col, int obj, int time, int LitIndex, int identity) : coord_x(row), coord_y(col), p(obj), t(time), x(LitIndex), identity(identity) {}
    Lit(const Lit &other) : coord_x(other.get_x()), coord_y(other.get_y()), p(other.get_p()), t(other.get_t()), x(other.x) {} // copy constructor
    // operator overloads
    bool operator==(const Lit &other) const { return this->x == other.x; }
    bool operator>(const Lit &other) const { return this->x > other.x; }
    bool operator<(const Lit &other) const { return this->x < other.x; }

    int get_x() const { return coord_x; }
    int get_y() const { return coord_y; }
    int get_p() const { return p; }
    int get_t() const { return t; }
    int get_identity() const { return identity; }

private:
    int coord_x;
    int coord_y;
    int p;
    int t;
    int identity; // 1 for player, 0 for box
};

inline Lit operator~(const Lit &p)
{
    Lit q = Lit(p);
    q.x = q.x * (-1); // 34 -> -34
    return q;
}
class Clause
{
public:
    void AddLit(const Lit &p);
    vector<Lit> clause;
};
// user defined hash function is necessary
class LitHash
{
public:
    // id is returned as hash function
    size_t operator()(const Lit &lit) const { return lit.x; }
};

class SokobanSolver
{
public:
    SokobanSolver();
    void setStepLimit(int limit);
    void loadMap(const string &fileName);
    // Constraints
    void PlayerMovementConstraints();        // 1
    void BoxPushMovementConstraints();       // 2
    void PlayerHeadOnConstraints();          // 3
    void PlayerSinglePlacementConstraints(); // 4
    void BoxSinglePlacementConstraints();    // 5
    void PlayerCollisionConstraints();       // 6
    void BoxCollisionConstraints();          // 7
    void BoxAndPlayerCollisionConstraints(); // 8
    void ExistenceConstraints();             // 9
    void ObstacleConstraints();              // 10

    // experimental constraints
    void LearntConstraints();

    // Initial State
    void InitState();
    // Solved State
    void SolvedState();

    void AddClause(Clause *newClause);
    Lit AddPlayerLiteral(int row, int col, int player, int time);
    Lit AddBoxLiteral(int row, int col, int box, int time);

    void AllConstraints();

    void CnfWriter(sat_solver *pSat);

    string createKey(int x, int y, int player, int time)
    {
        return to_string(x) + "_" + to_string(y) + "_" + to_string(player) + "_" + to_string(time);
    }

    void debugger(const string &fileName);

    pair<int, int> get_mapSize() { return mapSize; }
    Lit &get_LitDictionary(int index) { return LitDictionary[index]; }

    unordered_map<int, Lit> get_LitDictionary() { return LitDictionary; }
    unordered_map<string, vector<pair<int, int>>> &
    get_mapInfo() { return mapInfo; }

private:
    string mapName;
    int stepLimit;
    int LitIndex;                                          // initialize as 1
    unordered_map<string, vector<pair<int, int>>> mapInfo; // player, wall, block, target coordinate pairs
    vector<Clause *> clauses;
    pair<int, int> mapSize;
    unordered_map<string, pair<int, Lit>> playerLitManager; // (x,y, player, time) -> litIndex
    unordered_map<string, pair<int, Lit>> boxLitManager;    // (x,y, box, time) -> litIndex
    unordered_map<int, Lit> LitDictionary;
    int playerNum;
    int boxNum;
    ofstream outFile;
};

#endif
