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
#include "Preprocessor.h"
#include <fstream>
#include <iomanip>
using namespace std;

void visualizeSolution(Preprocessor &preprocessor, SokobanSolver &Solver, sat_solver *pSat, int step, bool verbose);
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
void WriteResultsToTable(const string &mapPath, bool timeout, double duration = 0.0, int steps = 0)
{
    static bool headerWritten = false;
    ios_base::openmode mode = headerWritten ? ios::app : ios::out; // Write mode for the first call, append mode afterward
    string mapName = mapPath.substr(mapPath.find_last_of("/\\") + 1);
    // Open the file in append mode
    ofstream outFile("table.txt", mode);

    // Check if the file is open
    if (!outFile.is_open())
    {
        cerr << "Error: Could not open table.txt for writing." << endl;
        return;
    }
    // Write the header if the file is empty
    if (!headerWritten)
    {
        outFile << left << setw(20) << "Map Name"
                << setw(15) << "Duration (s)"
                << setw(10) << "Steps"
                << endl;
        outFile << string(45, '-') << endl;
        headerWritten = true;
    }
    // Write the results
    outFile << left << setw(20) << mapName;
    if (timeout)
    {
        outFile << setw(15) << "N/A"
                << setw(10) << "N/A"
                << endl;
    }
    else
    {
        outFile << setw(15) << fixed << setprecision(3) << duration
                << setw(10) << steps
                << endl;
    }
    // Close the file
    outFile.close();
}
int Sokoban_CommandSolve(Abc_Frame_t *pAbc, int argc, char **argv)
{
    if (argc != 4)
    {
        cerr << "Usage: " << argv[0] << " <map file path> <run type> <verbose>" << std::endl;
        return 1;
    }
    const char *map = argv[1];
    int runType = atoi(argv[2]);
    int verbose = atoi(argv[3]);
    if (runType == 1)
    {
        using namespace std::chrono;
        auto start = high_resolution_clock::now();
        Preprocessor preprocessor(map, verbose);
        preprocessor.loadMap();
        preprocessor.TunnelIdentifying();
        preprocessor.findDeadlockPos();
        int step = 1;
        while (true)
        {
            auto curr_time = high_resolution_clock::now();
            auto TLE = hours(1);
            if (duration_cast<seconds>(curr_time - start) >= TLE)
            {
                cout << "Timeout: 1 hour" << endl;
                WriteResultsToTable(map, true);
                return 0;
            }
            SokobanSolver Solver(preprocessor);
            Solver.setStepLimit(step);
            Solver.verbose = verbose;
            sat_solver *pSat = sat_solver_new();
            Solver.AllConstraints();
            Solver.CnfWriter(pSat);

            int status = sat_solver_solve(pSat, nullptr, nullptr, 0, 0, 0, 0);
            if (status == l_True)
            {
                vector<int> true_literals;
                auto stop = high_resolution_clock::now();
                auto duration = duration_cast<microseconds>(stop - start);
                cout << "Solution found at: " << step << " steps" << endl;
                double duration_seconds = duration.count() / 1e6; // Convert microseconds to seconds

                // Round to three decimal places
                duration_seconds = round(duration_seconds * 1000) / 1000.0;
                cout << "BMC search duration: " << duration_seconds << " seconds" << endl;
                WriteResultsToTable(map, false, duration_seconds, step);

                if (!verbose)
                {
                    return 0;
                }
                visualizeSolution(preprocessor, Solver, pSat, step, verbose);
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
        Preprocessor preprocessor(map, verbose);
        preprocessor.loadMap();
        preprocessor.TunnelIdentifying();
        preprocessor.findDeadlockPos();
        int step = 1;
        int increment = 10;
        int foundStep = -1;

        while (true)
        {
            SokobanSolver Solver(preprocessor);
            Solver.setStepLimit(step);
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
        int low = max(1, step - (step <= 30 ? 10 : 8));
        int high = foundStep;

        while (low < high)
        {
            int mid = low + (high - low) / 2;

            SokobanSolver Solver(preprocessor);
            Solver.setStepLimit(mid);
            sat_solver *pSat = sat_solver_new();
            Solver.AllConstraints();
            Solver.CnfWriter(pSat);

            int status = sat_solver_solve(pSat, nullptr, nullptr, 0, 0, 0, 0);

            if (status == l_True)
                high = mid; // Narrow down to smaller step range
            else
                low = mid + 1; // Increase the lower bound
            sat_solver_delete(pSat);
        }
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(stop - start);

        cout << "Solution found at: " << low << " steps" << endl;
        cout << "BMC search duration: " << duration.count() << " seconds" << endl;
    }
    else if (runType == 3) // binary search with pull only constraints
    {
        using namespace std::chrono;
        auto start = high_resolution_clock::now();

        Preprocessor preprocessor(map, verbose);
        preprocessor.loadMap();
        preprocessor.TunnelIdentifying();
        preprocessor.findDeadlockPos();
        preprocessor.findPullRegions();
        int step = 100;
        int increment = 10;
        int foundStep = -1;

        while (true)
        {
            SokobanSolver Solver(preprocessor);
            cout << "step: " << step << endl;
            Solver.setStepLimit(step);
            sat_solver *pSat = sat_solver_new();
            Solver.PullOnlyConstraints();
            Solver.CnfWriter(pSat);

            int status = sat_solver_solve(pSat, nullptr, nullptr, 0, 0, 0, 0);

            if (status == l_True)
            {
                foundStep = step;
                visualizeSolution(preprocessor, Solver, pSat, foundStep, verbose);
                break;
            }

            sat_solver_delete(pSat);

            if (step < 5)
                step += 1;
            else if (step < 10)
                step += 5;
            else if (step < 30)
                step += 10; // Increment by 10 initially
            else
                step += 8; //  Increment by 8 after reaching step 30
        }

        // Perform binary search to find the minimum step
        /*int low = max(1, step - (step <= 30 ? 10 : 8));
        int high = foundStep;

        while (low < high)
        {
            int mid = low + (high - low) / 2;

            SokobanSolver Solver(preprocessor);
            Solver.setStepLimit(mid);
            sat_solver *pSat = sat_solver_new();
            Solver.PullOnlyConstraints();
            Solver.CnfWriter(pSat);

            int status = sat_solver_solve(pSat, nullptr, nullptr, 0, 0, 0, 0);

            if (status == l_True)
                high = mid; // Narrow down to smaller step range
            else
                low = mid + 1; // Increase the lower bound

            sat_solver_delete(pSat);
        }*/

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(stop - start);

        cout << "Solution found at: " << foundStep << " steps" << endl;
        cout << "BMC search duration: " << duration.count() << " seconds" << endl;
    }
    else if (runType == 4) // regular BMC with pull only constraints
    {
        using namespace std::chrono;
        auto start = high_resolution_clock::now();
        Preprocessor preprocessor(map, verbose);
        preprocessor.loadMap();
        preprocessor.TunnelIdentifying();
        preprocessor.findDeadlockPos();
        preprocessor.findPullRegions();
        int step = 1;
        while (true)
        {
            auto curr_time = high_resolution_clock::now();
            auto TLE = hours(1);
            if (duration_cast<seconds>(curr_time - start) >= TLE)
            {
                cout << "Timeout: 1 hour" << endl;
                WriteResultsToTable(map, true);
                return 0;
            }
            SokobanSolver Solver(preprocessor);
            Solver.setStepLimit(step);
            Solver.verbose = verbose;
            sat_solver *pSat = sat_solver_new();
            Solver.PullOnlyConstraints();
            Solver.CnfWriter(pSat);

            int status = sat_solver_solve(pSat, nullptr, nullptr, 0, 0, 0, 0);
            if (status == l_True)
            {
                vector<int> true_literals;
                auto stop = high_resolution_clock::now();
                auto duration = duration_cast<microseconds>(stop - start);
                cout << "Solution found at: " << step << " steps" << endl;
                double duration_seconds = duration.count() / 1e6; // Convert microseconds to seconds

                // Round to three decimal places
                duration_seconds = round(duration_seconds * 1000) / 1000.0;
                cout << "BMC search duration: " << duration_seconds << " seconds" << endl;
                WriteResultsToTable(map, false, duration_seconds, step);

                if (!verbose)
                {
                    return 0;
                }
                visualizeSolution(preprocessor, Solver, pSat, step, verbose);
                return 0;
            }
            sat_solver_delete(pSat);
            step++;
        }
    }
    else if (runType == 5) // regular BMC with pull only constraints
    {
        using namespace std::chrono;
        auto start = high_resolution_clock::now();
        Preprocessor preprocessor(map, verbose);
        preprocessor.loadMap();
        preprocessor.TunnelIdentifying();
        preprocessor.findDeadlockPos();
        int step = 1;
        int foundStep = -1;

        while (true)
        {
            SokobanSolver Solver(preprocessor);
            Solver.setStepLimit(step);
            sat_solver *pSat = sat_solver_new();
            Solver.PullOnlyConstraints();
            Solver.CnfWriter(pSat);

            int status = sat_solver_solve(pSat, nullptr, nullptr, 0, 0, 0, 0);

            if (status == l_True)
            {
                auto stop = high_resolution_clock::now();
                auto duration = duration_cast<seconds>(stop - start);
                foundStep = step;
                cout << "Solution found at: " << foundStep << " steps" << endl;
                cout << "BMC search duration: " << duration.count() << " seconds" << endl;
                visualizeSolution(preprocessor, Solver, pSat, foundStep, verbose);
                break;
            }

            sat_solver_delete(pSat);

            if (step < 20)
                step += 8; // Increment by 10 initially
            else
                step += 4; //  Increment by 8 after reaching step 30
        }
    }
}

void visualizeSolution(Preprocessor &preprocessor, SokobanSolver &Solver, sat_solver *pSat, int step, bool verbose)
{
    if (!verbose)
        return;

    // Get true literals from SAT solver
    vector<int> true_literals;
    const auto &litDict = Solver.get_LitDictionary();
    for (const auto &[index, lit] : litDict)
    {
        if (sat_solver_var_value(pSat, lit.x) > 0)
        {
            true_literals.push_back(lit.x);
        }
    }

    // ANSI color codes
    const struct
    {
        string black = "30";
        string red = "31";
        string green = "32";
        string yellow = "33";
        string blue = "34";
    } colors;

    cout << "Steps in action: " << endl;
    for (int t = 0; t <= step; t++)
    {
        // Initialize visualization grid
        auto visual = vector<vector<char>>(preprocessor.get_mapSize().first,
                                           vector<char>(preprocessor.get_mapSize().second, 'X'));

        // Place static elements
        for (const auto &[row, col] : preprocessor.get_mapInfo().at("Walls"))
            visual[row][col] = 'W';
        for (const auto &[row, col] : preprocessor.get_mapInfo().at("Targets"))
            visual[row][col] = 'T';

        // Place dynamic elements (players and boxes)
        for (int val : true_literals)
        {
            const auto &lit = Solver.get_LitDictionary(val);
            if (lit.get_t() == t)
            {
                visual[lit.get_x()][lit.get_y()] = lit.get_identity() == 1 ? 'P' : 'B';
            }
        }

        // Print the grid with colors
        for (const auto &row : visual)
        {
            for (char cell : row)
            {
                string color;
                switch (cell)
                {
                case 'B':
                    color = colors.green;
                    break;
                case 'P':
                    color = colors.blue;
                    break;
                case 'T':
                    color = colors.red;
                    break;
                case 'W':
                    color = colors.yellow;
                    break;
                default:
                    color = colors.black;
                }
                cout << "\033[" << color << "m" << cell << "\033[0m ";
            }
            cout << endl;
        }

        // Animation delay between steps
        if (t < step)
        {
            cout.flush();
            this_thread::sleep_for(chrono::seconds(1));
            cout << "\033[" << preprocessor.get_mapSize().first << "A";
        }
    }
}