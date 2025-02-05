#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "aig/aig/aig.h"
#include "iostream"
#include "set"
#include "queue"
#include "algorithm"
#include "sat/cnf/cnf.h"
#include "stdint.h"
#include <cstdlib>
#include <functional>
#include <unordered_set>
#include <thread> // For sleep
#include <chrono> // For time delays
#include "SokobanSolver.h"
using namespace std;

static int Sokoban_CommandSolve(Abc_Frame_t *pAbc, int argc, char **argv); // command function

void init(Abc_Frame_t *pAbc)
{
    Cmd_CommandAdd(pAbc, "LSV", "sokoban", Sokoban_CommandSolve, 0);
} // register this new command

void destroy(Abc_Frame_t *pAbc) {}

Abc_FrameInitializer_t frame_initializer = {init, destroy};

struct PackageRegistrationManager
{
    PackageRegistrationManager() { Abc_FrameAddInitializer(&frame_initializer); }
} lsvPackageRegistrationManager;
int Sokoban_CommandSolve(Abc_Frame_t *pAbc, int argc, char **argv)
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <map file path> <run type>" << std::endl;
        return 1;
    }
    const char *map = argv[1];
    int runType = atoi(argv[2]);
    if (runType == 1)
    {
        using namespace std::chrono;
        auto start = high_resolution_clock::now();
        int step = 1;
        while (true)
        {
            SokobanSolver Solver;
            Solver.setStepLimit(step);
            Solver.loadMap(map);
            sat_solver *pSat = sat_solver_new();
            Solver.AllConstraints();
            Solver.CnfWriter(pSat);

            // separate the constraints
            // increment steps, reuse previous clauses

            // Solver.debugger("Debug.txt");
            // cout << "Finished" << endl;

            // color code
            string black = "30";
            string red = "31";
            string green = "32"; // box color
            string yellow = "33";
            string blue = "34";

            int status = sat_solver_solve(pSat, nullptr, nullptr, 0, 0, 0, 0);
            if (status == l_True)
            {
                vector<int> true_literals;
                auto stop = high_resolution_clock::now();
                auto duration = duration_cast<seconds>(stop - start);
                cout << "Solution found at: " << step << " steps" << endl;
                cout << "BMC search duration: " << duration.count() << " seconds" << endl;

                unordered_map<int, Lit> LitDictionary = Solver.get_LitDictionary();
                for (int index = 0; index < LitDictionary.size(); index++)
                {
                    int value = sat_solver_var_value(pSat, LitDictionary[index].x);
                    if (value > 0)
                        true_literals.push_back(LitDictionary[index].x);
                }

                cout << "Steps in action: " << endl;
                for (int t = 0; t <= step; t++)
                {
                    vector<vector<char>> visual(Solver.get_mapSize().first, vector<char>(Solver.get_mapSize().second, 'X'));
                    for (const auto &wall_coord : Solver.get_mapInfo()["Walls"])
                        visual[wall_coord.first][wall_coord.second] = 'W';
                    for (const auto &target_coord : Solver.get_mapInfo()["Targets"])
                        visual[target_coord.first][target_coord.second] = 'T';
                    for (auto val : true_literals)
                    {
                        Lit &lit = Solver.get_LitDictionary(val);
                        if (lit.get_t() == t)
                        {
                            if (lit.get_identity() == 1)
                                visual[lit.get_x()][lit.get_y()] = 'P';
                            else
                                visual[lit.get_x()][lit.get_y()] = 'B';
                        }
                    }
                    // print
                    for (int row = 0; row < Solver.get_mapSize().first; row++)
                    {
                        for (int col = 0; col < Solver.get_mapSize().second; col++)
                        {
                            switch (visual[row][col])
                            {
                            case 'B':
                                cout << "\033[" << green << "m" << visual[row][col] << "\033[0m" << " ";
                                break;
                            case 'P':
                                cout << visual[row][col] << " ";
                                break;
                            case 'T':
                                cout << "\033[" << red << "m" << visual[row][col] << "\033[0m" << " ";
                                break;
                            case 'W':
                                cout << "\033[" << yellow << "m" << visual[row][col] << "\033[0m" << " ";
                                break;
                            default:
                                cout << "\033[" << black << "m" << visual[row][col] << "\033[0m" << " ";
                                break;
                            }
                        }
                        cout << endl;
                    }
                    if (t < step)
                    {
                        cout.flush();
                        this_thread::sleep_for(chrono::seconds(1));
                        cout << "\033[" << Solver.get_mapSize().first << "A";
                    }
                }
                return 0;
            }
            sat_solver_delete(pSat);
            step++;
        }
    }
    else if (runType == 2) // binary search
    {
        using namespace std::chrono;
        auto start = high_resolution_clock::now();

        int step = 1;
        int increment = 10;
        int foundStep = -1;

        while (true)
        {
            SokobanSolver Solver;
            Solver.setStepLimit(step);
            Solver.loadMap(map);

            sat_solver *pSat = sat_solver_new();
            Solver.AllConstraints();
            Solver.CnfWriter(pSat);

            int status = sat_solver_solve(pSat, nullptr, nullptr, 0, 0, 0, 0);

            if (status == l_True)
            {
                foundStep = step;
                break;
            }

            sat_solver_delete(pSat);

            if (step < 30)
                step += 10; // Increment by 10 initially
            else
                step += 8; //  Increment by 8 after reaching step 30
        }

        // Perform binary search to find the minimum step
        int low = step - (step <= 30 ? 10 : 8);
        int high = foundStep;

        while (low < high)
        {
            int mid = low + (high - low) / 2;

            SokobanSolver Solver;
            Solver.setStepLimit(mid);
            Solver.loadMap(map);

            sat_solver *pSat = sat_solver_new();
            Solver.AllConstraints();
            Solver.CnfWriter(pSat);

            int status = sat_solver_solve(pSat, nullptr, nullptr, 0, 0, 0, 0);

            if (status == l_True)
            {
                high = mid; // Narrow down to smaller step range
            }
            else
            {
                low = mid + 1; // Increase the lower bound
            }

            sat_solver_delete(pSat);
        }
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(stop - start);

        cout << "Solution found at: " << low << " steps" << endl;
        cout << "BMC search duration: " << duration.count() << " seconds" << endl;
    }
    return 0;
}
