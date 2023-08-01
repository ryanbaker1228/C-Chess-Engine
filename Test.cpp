//
// Created by Ryan Baker on 7/31/23.
//

#include "Test.h"
#include "gamestate.h"
#include "movegen.h"
#include <iostream>
#include <iomanip>


void MoveGenTest::PerftTest(Level level) {
    Gamestate &gamestate = Gamestate::Get();
    MoveGenerator &generator = MoveGenerator::Get();

    std::vector<int> testPositions, testingDepths;
    int numPositions, maxNodes, depth, position;
    U64 totalTime = 0, totalNodes = 0, nodesFound;
    switch(level) {
        case QuickTest:
            numPositions = 32;
            maxNodes = 1000000; // 1 million

            testPositions.reserve(numPositions);
            testingDepths.reserve(numPositions);

            while (testPositions.size() != numPositions) {
                position = rand() % 128;
                if (std::find(testPositions.begin(), testPositions.end(), position) != testPositions.end()) continue;
                depth = 0;
                while (perftResults[position][depth + 1] < maxNodes) {
                    depth++;
                    if (depth == 5) break;
                }
                testPositions.push_back(position);
                testingDepths.push_back(depth);
            }
            break;

        case StandardTest:
        case FullTest:
        default:
            numPositions = 128;
            depth = 5;

            testPositions.reserve(numPositions);
            testingDepths.reserve(numPositions);

            for (position = 0; position < numPositions; ++position) {
                testPositions.push_back(position);
                testingDepths.push_back(depth);
                if (position == 1) testingDepths.back() -= 1; //Test 2 at a depth of 6 has more nodes than the int type
            }
            break;

    }
    while (!testPositions.empty()) {
        position = testPositions.front();
        depth = testingDepths.front();

        gamestate.Seed(positions[position]);

        auto start = std::chrono::high_resolution_clock::now();
        nodesFound = generator.PerftTree(depth + 1);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        totalNodes += nodesFound;
        totalTime += duration.count();

        OutputTestResult(nodesFound == perftResults[position][depth], float(duration.count()) / pow(10, 6), nodesFound, position, depth);

        testPositions.erase(testPositions.begin());
        testingDepths.erase(testingDepths.begin());
    }
    std::cout << "Total Time: " << double(totalTime) / pow(10, 6) << " seconds" << std::endl;
    std::cout << "Average NPS: " << double(totalNodes) / (double(totalTime) / pow(10, 6)) << std::endl;
}

void MoveGenTest::OutputTestResult(bool passed, float time, int nodes, int testNum, int depth) {
    using namespace std;

    string testResult = passed ? " passed" : " failed";

    setprecision(3);
    cout << "Test " << setw(3) << testNum + 1 << testResult << "    ";
    cout << setw(9) << time << " seconds    ";
    cout << setw(9) << perftResults[testNum][depth] << " nodes expected    ";
    cout << setw(9) << nodes << " nodes found    ";
    cout << setw(12) << float(nodes) / time << " NPS" << endl;
}