#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <vector>
#include <utility>
#include <set>
#include <unordered_map>
#include <queue>
#include <map>

using namespace std;

class Preprocessor
{
public:
    Preprocessor(const string &filename, bool verbose = false);
    void loadMap();

    /*
    ============ Pull Stage ============
    */
    void findPullRegions();
    const vector<vector<pair<int, int>>> &getPullableRegions() const { return pullableRegions; }
    bool isPullablePosition(int row, int col) const;
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
    inline bool notWall(int row, int col) const { return find(mapInfo.at("Walls").begin(), mapInfo.at("Walls").end(), make_pair(row, col)) == mapInfo.at("Walls").end(); }
    inline bool isWall(int row, int col) const { return find(mapInfo.at("Walls").begin(), mapInfo.at("Walls").end(), make_pair(row, col)) != mapInfo.at("Walls").end(); }
    inline bool isWalkable(int row, int col) const { return find(mapInfo.at("Walkable").begin(), mapInfo.at("Walkable").end(), make_pair(row, col)) != mapInfo.at("Walkable").end(); }
    inline bool isTarget(int row, int col) const { return mapInfo.at("Targets").end() != find(mapInfo.at("Targets").begin(), mapInfo.at("Targets").end(), make_pair(row, col)); }
    inline bool isDeadLockBoxPos(int row, int col) const { return deadlockPosMap[row][col]; }

private:
    string filename;
    vector<vector<char>> map;
    unordered_map<string, vector<pair<int, int>>> mapInfo;
    vector<vector<pair<int, int>>> pullableRegions; // Direct vector of regions
    vector<vector<bool>> pullablePosMap;            // Fast lookup map for pullable positions
    set<pair<int, int>> pullable_set;
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