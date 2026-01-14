#include <iostream>
#include <vector>
#include <deque>
#include <limits>
#include <string>
#include <fstream>

using namespace std;

// ================== Direction ==================

enum class Direction {
    Idle,
    Up,
    Down
};

string directionToString(Direction d) {
    switch (d) {
        case Direction::Idle: return "Idle";
        case Direction::Up:   return "Up";
        case Direction::Down: return "Down";
    }
    return "Unknown";
}

// ================== Request ==================

class Request {
public:
    int fromFloor;
    int toFloor;
    int timeRequested;

    Request(int from, int to, int t)
        : fromFloor(from), toFloor(to), timeRequested(t) {}
};

// ================== Elevator ==================

class Elevator {
private:
    int id;
    int currentFloor;
    Direction direction;
    bool doorOpen;
    deque<int> targets;      // floors to visit (queue)
    int totalStopsServed;

public:
    Elevator(int id_, int startFloor = 0)
        : id(id_), currentFloor(startFloor),
          direction(Direction::Idle), doorOpen(false),
          totalStopsServed(0) {}

    int getId() const { return id; }
    int getCurrentFloor() const { return currentFloor; }
    Direction getDirection() const { return direction; }
    bool isDoorOpen() const { return doorOpen; }
    int getQueueSize() const { return static_cast<int>(targets.size()); }
    int getTotalStopsServed() const { return totalStopsServed; }

    void addTarget(int floor) {
        if (!targets.empty() && targets.back() == floor) {
            return; // avoid duplicate consecutive target
        }
        targets.push_back(floor);
    }

    bool isIdle() const {
        return targets.empty() && !doorOpen && direction == Direction::Idle;
    }

    int distanceToFloor(int floor) const {
        return abs(floor - currentFloor);
    }

    /*
       step() simulates one time unit:
       - If door is open → close it and finish the stop
       - If no targets → stay idle
       - Otherwise → move one floor toward next target
    */
    void step() {
        // If door is open, close it and complete this stop
        if (doorOpen) {
            doorOpen = false;
            ++totalStopsServed;

            if (!targets.empty() && targets.front() == currentFloor) {
                targets.pop_front();
            }

            if (targets.empty()) {
                direction = Direction::Idle;
            }
            return;
        }

        // No targets -> stay idle
        if (targets.empty()) {
            direction = Direction::Idle;
            return;
        }

        // Move toward the first target in the queue
        int target = targets.front();

        if (currentFloor < target) {
            ++currentFloor;
            direction = Direction::Up;
        }
        else if (currentFloor > target) {
            --currentFloor;
            direction = Direction::Down;
        }
        else {
            // Arrived at target -> open door
            doorOpen = true;
        }
    }

    void printStatus() const {
        cout << "Elevator " << id
             << " | Floor: " << currentFloor
             << " | Dir: " << directionToString(direction)
             << " | Door: " << (doorOpen ? "Open" : "Closed")
             << " | Queue size: " << targets.size()
             << '\n';
    }

    void logStatus(ofstream& log, int timeStep) const {
        log << "t=" << timeStep
            << " Elevator " << id
            << " Floor=" << currentFloor
            << " Dir=" << directionToString(direction)
            << " Door=" << (doorOpen ? "Open" : "Closed")
            << " QueueSize=" << targets.size()
            << "\n";
    }
};

// ================== ElevatorSystem ==================

class ElevatorSystem {
private:
    int numFloors;
    vector<Elevator> elevators;
    vector<Request> pendingRequests;
    int currentTime;
    ofstream logFile;
    int totalRequestsProcessed;

    // Direction-aware assignment of requests to elevators
    void assignRequests() {
        vector<Request> stillPending;

        for (const auto& req : pendingRequests) {
            int bestIndex = -1;
            int bestScore = numeric_limits<int>::max();

            for (size_t i = 0; i < elevators.size(); ++i) {
                const Elevator& e = elevators[i];

                int dist = e.distanceToFloor(req.fromFloor);
                int score = dist;

                Direction dir = e.getDirection();
                int elevFloor = e.getCurrentFloor();

                bool goingSameWay = false;

                if (dir == Direction::Idle) {
                    goingSameWay = true; // idle can go anywhere
                }
                else if (dir == Direction::Up && req.fromFloor >= elevFloor) {
                    goingSameWay = true;
                }
                else if (dir == Direction::Down && req.fromFloor <= elevFloor) {
                    goingSameWay = true;
                }

                // Penalty if going opposite direction
                if (!goingSameWay && dir != Direction::Idle) {
                    score += 5;
                }

                // Prefer shorter queues a bit
                score += e.getQueueSize();

                if (score < bestScore) {
                    bestScore = score;
                    bestIndex = static_cast<int>(i);
                }
            }

            if (bestIndex != -1) {
                Elevator& chosen = elevators[bestIndex];
                // First go to pickup, then to destination
                chosen.addTarget(req.fromFloor);
                chosen.addTarget(req.toFloor);
                ++totalRequestsProcessed;
            } else {
                stillPending.push_back(req);
            }
        }

        pendingRequests = stillPending;
    }

  
    // Print the vertical building view (ASCII-safe version)

    void printBuildingView() const {
        cout << "Building view (top = highest floor)\n\n";

        for (int floor = numFloors - 1; floor >= 0; --floor) {
            cout << "Floor " << floor << " | ";

            for (const auto& e : elevators) {
                if (e.getCurrentFloor() == floor) {
                    char dirChar = 'I';
                    switch (e.getDirection()) {
                        case Direction::Up:   dirChar = 'U'; break;
                        case Direction::Down: dirChar = 'D'; break;
                        case Direction::Idle: dirChar = 'I'; break;
                    }

                    string doorText = e.isDoorOpen() ? "Open" : "Closed";

                    // Example: [E0 U Open]
                    cout << "[E" << e.getId() << " " << dirChar << " " << doorText << "]";
                } else {
                    cout << "[            ]";
                }
            }
            cout << "\n";
        }

        cout << "\nLegend: U=Up, D=Down, I=Idle, Door: Open/Closed\n\n";
    }



public:
    ElevatorSystem(int floors, int numElevators)
        : numFloors(floors),
          currentTime(0),
          totalRequestsProcessed(0)
    {
        for (int i = 0; i < numElevators; ++i) {
            elevators.emplace_back(i, 0); // all start at floor 0
        }

        logFile.open("elevator_log.txt");
        if (logFile.is_open()) {
            logFile << "Elevator Simulation Log\n";
        }
    }

    ~ElevatorSystem() {
        if (logFile.is_open()) {
            logFile << "Simulation ended. Total time steps: "
                    << currentTime << "\n";
            logFile.close();
        }
    }

    int getNumFloors() const { return numFloors; }
    int getCurrentTime() const { return currentTime; }

    void addRequest(int fromFloor, int toFloor) {
        if (fromFloor < 0 || fromFloor >= numFloors ||
            toFloor   < 0 || toFloor   >= numFloors) {
            cout << "Invalid request. Floors must be between 0 and "
                 << numFloors - 1 << ".\n";
            return;
        }
        if (fromFloor == toFloor) {
            cout << "You are already on that floor.\n";
            return;
        }

        pendingRequests.emplace_back(fromFloor, toFloor, currentTime);
        cout << "Request added from floor " << fromFloor
             << " to floor " << toFloor << ".\n";
    }

    void step() {
        ++currentTime;

        assignRequests();

        for (auto& elevator : elevators) {
            elevator.step();
        }

        if (logFile.is_open()) {
            for (const auto& elevator : elevators) {
                elevator.logStatus(logFile, currentTime);
            }
        }
    }

    void printStatus() const {
        cout << "\n=== Time step: " << currentTime << " ===\n";

        // Visual building representation
        printBuildingView();

        // Detailed per-elevator info
        cout << "Elevator details:\n";
        for (const auto& elevator : elevators) {
            elevator.printStatus();
        }

        cout << "Pending requests: " << pendingRequests.size() << "\n";
    }

    void printSummary() const {
        cout << "\n===== Simulation Summary =====\n";
        cout << "Total time steps: " << currentTime << "\n";
        cout << "Total requests processed (assigned): " << totalRequestsProcessed << "\n";
        for (const auto& e : elevators) {
            cout << "Elevator " << e.getId()
                 << " served stops: " << e.getTotalStopsServed() << "\n";
        }
        cout << "Log saved to elevator_log.txt (if file I/O is allowed).\n";
    }
};

// ================== Helper ==================

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// ================== main ==================

int main() {
    cout << "===== Elevator Simulation =====\n";

    int floors;
    int numElevators;

    cout << "Enter number of floors (5 - 20): ";
    cin >> floors;
    if (!cin || floors < 5 || floors > 20) {
        cout << "Invalid input. Defaulting to 10 floors.\n";
        clearInput();
        floors = 10;
    }

    cout << "Enter number of elevators (1 - 5): ";
    cin >> numElevators;
    if (!cin || numElevators < 1 || numElevators > 5) {
        cout << "Invalid input. Defaulting to 2 elevators.\n";
        clearInput();
        numElevators = 2;
    }

    ElevatorSystem elevatorSystem(floors, numElevators);

    char command = ' ';
    bool running = true;

    while (running) {
        elevatorSystem.printStatus();

        cout << "\nOptions:\n";
        cout << "  r - new request (simulate a person calling elevator)\n";
        cout << "  s - advance simulation by 1 time step\n";
        cout << "  a - auto-run 5 steps\n";
        cout << "  q - quit simulation\n";
        cout << "Enter command: ";
        cin >> command;

        switch (command) {
            case 'r':
            case 'R': {
                int from, to;

                cout << "Enter current floor: ";
                cin >> from;

                cout << "Enter destination floor: ";
                cin >> to;

                elevatorSystem.addRequest(from, to);
                break;
            }

            case 's':
            case 'S':
                elevatorSystem.step();
                break;

            case 'a':
            case 'A': {
                int steps = 5;
                cout << "Auto-running " << steps << " steps...\n";
                for (int i = 0; i < steps; ++i) {
                    elevatorSystem.step();
                }
                break;
            }

            case 'q':
            case 'Q':
                running = false;
                break;

            default:
                cout << "Invalid command.\n";
                clearInput();
                break;
        }
    }

    elevatorSystem.printSummary();
    cout << "Goodbye!\n";

    return 0;
}
