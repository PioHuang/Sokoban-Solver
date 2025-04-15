#include "Preprocessor.h"
#include <fstream>
#include <iostream>
#include <queue>
#include <algorithm>

Preprocessor::Preprocessor(const string &filename, bool verbose) : filename(filename), verbose(verbose) {}

void Preprocessor::loadMap()
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
    findDeadlockPos();
}

void Preprocessor::findDeadlockPos()
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
    deadlockPositions.clear();
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
                        deadlockPositions.push_back(pos);
                    }
                }
                deadGroupCount++;
            }
        }
    }

    if (verbose)
        cout << "Num of dead lock box positions: " << deadlockPositions.size() << endl;
    vector<vector<char>> deadLockBoxPosStr(mapSize.first, vector<char>(mapSize.second, ' '));
    // deadLockBoxPosMap
    deadlockPosMap = vector<vector<bool>>(mapSize.first, vector<bool>(mapSize.second, false));
    for (const auto &d : deadlockPositions)
    {
        int i = d.first;
        int j = d.second;
        deadlockPosMap[i][j] = true;
    }
}
void Preprocessor::bfs(vector<vector<char>> &underlyingTiles, vector<vector<bool>> &visited, vector<pair<int, int>> &group, int i, int j)
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

void Preprocessor::TunnelIdentifying()
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
            visited[row][col] = true;
            // Check for horizontal tunnel
            if (isWall(row - 1, col) && isWall(row + 1, col))
            {
                vector<pair<int, int>> t;
                // Check if left and right are walkable
                bool left_deadend = false;
                if (!isWalkable(row, col - 1))
                    left_deadend = true;
                if (col > 0 && col < mapSize.second && isWalkable(row, col + 1))
                {
                    tunnel_map[row][col] = 1; // tunnel entry
                    if (helper(tunnel_map, visited, row, col + 1, 1, 0, t))
                        t.push_back({row, col});
                }
                if (t.size() != 0 && !left_deadend)
                    tunnels.insert(t);
            }
            // Check for vertical tunnel
            if (isWall(row, col - 1) && isWall(row, col + 1))
            {
                vector<pair<int, int>> t;
                // Check if up and down are walkable
                bool top_deadend = false;
                if (!isWalkable(row - 1, col))
                    top_deadend = true;
                if ((row > 0 && isWalkable(row - 1, col)) &&
                    (row < mapSize.first && isWalkable(row + 1, col)))
                {
                    tunnel_map[row][col] = 1; // tunnel entry
                    if (helper(tunnel_map, visited, row + 1, col, 1, 1, t))
                        t.push_back({row, col});
                }
                if (t.size() != 0 && !top_deadend)
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
        cout << "Size of tunnels: " << tunnels.size() << endl;
    }
}
bool Preprocessor::helper(vector<vector<int>> &tunnel_map, vector<vector<bool>> &visited, int row, int col, int index, int type, vector<pair<int, int>> &t)
{ // dfs for tunnels
    visited[row][col] = true;
    // base case
    if (type == 0) // horizontal
    {
        if (isWall(row, col + 1) && isWall(row + 1, col) && isWall(row - 1, col))
            return false;
        if ((notWall(row - 1, col) || notWall(row + 1, col)))
        {
            if (index >= 2)
            {
                t.push_back({row, col - 1});
                return true;
            }
            else
                return false;
            ;
        }
        return helper(tunnel_map, visited, row, col + 1, index + 1, 0, t);
    }
    else
    { // vertical
        if (isWall(row + 1, col) && isWall(row, col + 1) && isWall(row, col - 1))
            return false;
        if (notWall(row, col + 1) || notWall(row, col - 1))
        {
            if (index >= 2)
            {
                t.push_back({row - 1, col});
                return true;
            }
            else
                return false;
        }
        return helper(tunnel_map, visited, row + 1, col, index + 1, 1, t);
    }
    tunnel_map[row][col] = index + 1;
}
