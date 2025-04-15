#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <vector>
#include <utility>
#include <set>

using namespace std;

class Preprocessor
{
public:
    Preprocessor(const string &filename, bool verbose = false);
    void loadMap();
    /*
    ============ Tunnel Macro ============
    */
    void TunnelIdentifying();
    bool helper(vector<vector<int>> &tunnel_map, vector<vector<bool>> &visited, int row, int col, int index, int type, vector<pair<int, int>> &t);
    const set<vector<pair<int, int>>> &getTunnels() const { return tunnels; };
    /*
    ============ Deadend Positions ============
    */
    void findDeadlockPos();
    void bfs(vector<vector<char>> &underlyingTiles, vector<vector<bool>> &visited, vector<pair<int, int>> &group, int i, int j);
    const vector<pair<int, int>> &getDeadlockPositions() const { return deadlockPositions; }
    /*
    ============ Getters & Conditioners ============
    */
    const unordered_map<string, vector<pair<int, int>>> &get_mapInfo() const { return mapInfo; };
    const pair<int, int> get_mapSize() const { return mapSize; };
    inline bool notWall(int row, int col) { return find(mapInfo["Walls"].begin(), mapInfo["Walls"].end(), make_pair(row, col)) == mapInfo["Walls"].end(); }
    inline bool isWall(int row, int col) { return find(mapInfo["Walls"].begin(), mapInfo["Walls"].end(), make_pair(row, col)) != mapInfo["Walls"].end(); }
    inline bool isWalkable(int row, int col) { return find(mapInfo["Walkable"].begin(), mapInfo["Walkable"].end(), make_pair(row, col)) != mapInfo["Walkable"].end(); }
    inline bool isTarget(int row, int col) { return mapInfo["Targets"].end() != find(mapInfo["Targets"].begin(), mapInfo["Targets"].end(), make_pair(row, col)); }
    inline bool isDeadLockBoxPos(int row, int col) { return deadlockPosMap[row][col]; }

private:
    string filename;
    vector<vector<char>> map;
    unordered_map<string, vector<pair<int, int>>> mapInfo;
    set<vector<pair<int, int>>> tunnels = {};
    vector<pair<int, int>> deadlockPositions;
    vector<vector<bool>> deadlockPosMap;
    pair<int, int> mapSize;
    int playerNum;
    int boxNum;
    bool verbose;

    friend class SokobanSolver;
};

#endif // PREPROCESSOR_H
