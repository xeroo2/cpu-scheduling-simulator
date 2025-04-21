#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <queue>
#include <unordered_map>
#include <tuple>
using namespace std;

struct Process
{
    int id; // Process ID
    int arrival_time;
    int burst_time;
    int priority; // Only for Priority Scheduling
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int remaining_time; // For SRT and RR
};

void printGanttChart(const vector<tuple<int, int, int>> &gantt_chart)
{
    cout << "\n==================== Gantt Chart ====================\n";
    cout << "| ";
    for (const auto &seg : gantt_chart)
    {
        int pid;
        tie(pid, ignore, ignore) = seg;
        if (pid == -1)
            cout << "IDLE | ";
        else
            cout << "P" << pid << " | ";
    }
    cout << "\n------------------------------------------------------\n";
    if (!gantt_chart.empty())
    {
        cout << get<1>(gantt_chart[0]);
        for (const auto &seg : gantt_chart)
        {
            int et;
            tie(ignore, ignore, et) = seg;
            cout << "    " << et;
        }
    }
    cout << "\n======================================================\n\n";
}

void printProcessMetrics(const vector<Process> &processes,
                         const string &metricName,
                         float average)
{
    cout << "==================== " << metricName << " ====================\n";
    cout << left << setw(10) << "Process"
         << right << setw(20) << metricName << "\n";
    cout << "------------------------------------------------------\n";
    for (const auto &p : processes)
    {
        cout << left << setw(10) << ("P" + to_string(p.id))
             << right << setw(20) << (metricName == "Turnaround Time" ? p.turnaround_time : p.waiting_time)
             << "\n";
    }
    cout << "------------------------------------------------------\n";
    cout << left << setw(10) << "Average"
         << right << setw(20) << fixed << setprecision(2)
         << average << "\n";
    cout << "======================================================\n\n";
}

void sjn(vector<Process> &processes, int num_processes)
{
    // Sort processes based on arrival time and burst time
    sort(processes.begin(), processes.end(), [](Process a, Process b)
         {
        if (a.arrival_time == b.arrival_time)
            return a.burst_time < b.burst_time;
        return a.arrival_time < b.arrival_time; });

    vector<Process> completed_processes;
    vector<tuple<int, int, int>> gantt_chart; // Declare gantt_chart here
    int current_time = 0;
    float total_turnaround_time = 0, total_waiting_time = 0;

    while (!processes.empty())
    {
        auto shortest_job = processes.end();
        for (auto it = processes.begin(); it != processes.end(); ++it)
        {
            if (it->arrival_time <= current_time)
            {
                if (shortest_job == processes.end() || it->burst_time < shortest_job->burst_time)
                {
                    shortest_job = it;
                }
            }
        }

        if (shortest_job != processes.end())
        {
            gantt_chart.emplace_back(shortest_job->id, current_time, current_time + shortest_job->burst_time);
            current_time += shortest_job->burst_time;
            shortest_job->completion_time = current_time;
            shortest_job->turnaround_time = shortest_job->completion_time - shortest_job->arrival_time;
            shortest_job->waiting_time = shortest_job->turnaround_time - shortest_job->burst_time;

            total_turnaround_time += shortest_job->turnaround_time;
            total_waiting_time += shortest_job->waiting_time;

            completed_processes.push_back(*shortest_job);
            processes.erase(shortest_job);
        }
        else
        {
            int next_arrival = min_element(processes.begin(), processes.end(),
                                           [](const Process &a, const Process &b)
                                           { return a.arrival_time < b.arrival_time; })
                                   ->arrival_time;
            gantt_chart.emplace_back(-1, current_time, next_arrival);
            current_time = next_arrival;
        }
    }

    printGanttChart(gantt_chart);

    float average_turnaround = total_turnaround_time / num_processes;
    float average_waiting = total_waiting_time / num_processes;

    printProcessMetrics(completed_processes, "Turnaround Time", average_turnaround);

    printProcessMetrics(completed_processes, "Waiting Time", average_waiting);
}

void nonPreemptivePriority(vector<Process> processes)
{
    int current_time = 0;
    vector<Process> completed;
    vector<tuple<int, int, int>> gantt_chart;

    while (!processes.empty())
    {
        vector<Process *> available;
        for (auto &p : processes)
        {
            if (p.arrival_time <= current_time)
            {
                available.push_back(&p);
            }
        }

        Process *selected = nullptr;
        if (!available.empty())
        {
            selected = *min_element(available.begin(), available.end(), [](Process *a, Process *b)
                                    { return a->priority < b->priority; });
        }
        else
        {
            selected = &(*min_element(processes.begin(), processes.end(), [](Process &a, Process &b)
                                      { return a.arrival_time < b.arrival_time; }));
            // **Record CPU Idle Time in Gantt Chart**
            gantt_chart.emplace_back(-1, current_time, selected->arrival_time);
            current_time = selected->arrival_time;
        }

        gantt_chart.emplace_back(selected->id, current_time, current_time + selected->burst_time);

        selected->completion_time = current_time + selected->burst_time;
        selected->turnaround_time = selected->completion_time - selected->arrival_time;
        selected->waiting_time = selected->turnaround_time - selected->burst_time;

        completed.push_back(*selected);
        current_time = selected->completion_time;

        processes.erase(remove_if(processes.begin(), processes.end(), [&](Process &p)
                                  { return p.id == selected->id; }),
                        processes.end());
    }

    printGanttChart(gantt_chart);

    float total_turnaround_time = 0, total_waiting_time = 0;
    for (const auto &p : completed)
    {
        total_turnaround_time += p.turnaround_time;
        total_waiting_time += p.waiting_time;
    }
    float average_turnaround = total_turnaround_time / completed.size();
    float average_waiting = total_waiting_time / completed.size();

    printProcessMetrics(completed, "Turnaround Time", average_turnaround);

    printProcessMetrics(completed, "Waiting Time", average_waiting);
}

void roundRobin(vector<Process> processes, int quantum)
{
    sort(processes.begin(), processes.end(),
         [](const Process &a, const Process &b)
         {
             return a.arrival_time < b.arrival_time;
         });

    queue<Process *> ready_queue;
    vector<tuple<int, int, int>> gantt_chart;

    vector<Process> completed_processes;
    int current_time = 0;
    float total_waiting_time = 0, total_turnaround_time = 0;

    int index = 0;
    while (index < (int)processes.size() && processes[index].arrival_time <= current_time)
    {
        ready_queue.push(&processes[index]);
        index++;
    }

    while (!ready_queue.empty() || index < (int)processes.size())
    {
        if (ready_queue.empty())
        {
            int next_arrival = processes[index].arrival_time;
            if (!gantt_chart.empty())
            {
                int last_end = get<2>(gantt_chart.back());
                if (last_end < next_arrival)
                {
                    gantt_chart.emplace_back(-1, last_end, next_arrival);
                }
            }
            else
            {
                if (current_time < next_arrival)
                {
                    gantt_chart.emplace_back(-1, current_time, next_arrival);
                }
            }
            current_time = next_arrival;

            while (index < (int)processes.size() &&
                   processes[index].arrival_time <= current_time)
            {
                ready_queue.push(&processes[index]);
                index++;
            }
        }
        else
        {
            Process *current_process = ready_queue.front();
            ready_queue.pop();

            int start_time = current_time;
            int slice = min(quantum, current_process->remaining_time);
            int end_time = start_time + slice;

            gantt_chart.emplace_back(current_process->id, start_time, end_time);
            current_time = end_time;
            current_process->remaining_time -= slice;

            while (index < (int)processes.size() &&
                   processes[index].arrival_time <= current_time)
            {
                ready_queue.push(&processes[index]);
                index++;
            }

            if (current_process->remaining_time > 0)
            {
                ready_queue.push(current_process);
            }
            else
            {
                current_process->completion_time = current_time;
                current_process->turnaround_time =
                    current_process->completion_time - current_process->arrival_time;
                current_process->waiting_time =
                    current_process->turnaround_time - current_process->burst_time;

                total_waiting_time += current_process->waiting_time;
                total_turnaround_time += current_process->turnaround_time;
                completed_processes.push_back(*current_process);
            }
        }
    }

    float average_turnaround = total_turnaround_time / completed_processes.size();
    float average_waiting = total_waiting_time / completed_processes.size();

    printGanttChart(gantt_chart);
    printProcessMetrics(completed_processes, "Turnaround Time", average_turnaround);
    printProcessMetrics(completed_processes, "Waiting Time", average_waiting);
}

void srt(vector<Process> processes)
{
    sort(processes.begin(), processes.end(),
         [](const Process &a, const Process &b)
         {
             return a.arrival_time < b.arrival_time;
         });

    vector<tuple<int, int, int>> gantt_chart;
    int current_time = 0;

    float total_waiting_time = 0, total_turnaround_time = 0;
    int n = processes.size();
    int completed_count = 0;
    int last_pid = -2;
    int segment_start_time = 0;

    vector<Process> completed_processes;

    while (completed_count < n)
    {
        int idx = -1;
        int best_rem = INT_MAX;

        for (int i = 0; i < (int)processes.size(); i++)
        {
            if (processes[i].remaining_time > 0 &&
                processes[i].arrival_time <= current_time)
            {
                if (processes[i].remaining_time < best_rem)
                {
                    best_rem = processes[i].remaining_time;
                    idx = i;
                }
            }
        }

        if (idx == -1)
        {
            if (last_pid != -1)
            {
                if (last_pid >= 1 && !gantt_chart.empty())
                {
                    get<2>(gantt_chart.back()) = current_time;
                }

                segment_start_time = current_time;
                gantt_chart.emplace_back(-1, segment_start_time, segment_start_time + 1);
                last_pid = -1;
            }
            else
            {
                get<2>(gantt_chart.back()) = current_time + 1;
            }
            current_time++;
        }
        else
        {
            Process &p = processes[idx];

            if (p.id != last_pid)
            {
                if (last_pid != -2 && !gantt_chart.empty())
                {
                    get<2>(gantt_chart.back()) = current_time;
                }
                segment_start_time = current_time;
                gantt_chart.emplace_back(p.id, segment_start_time, segment_start_time + 1);
                last_pid = p.id;
            }
            else
            {
                get<2>(gantt_chart.back()) = current_time + 1;
            }

            p.remaining_time--;
            current_time++;

            if (p.remaining_time == 0)
            {
                p.completion_time = current_time;
                p.turnaround_time = p.completion_time - p.arrival_time;
                p.waiting_time = p.turnaround_time - p.burst_time;

                total_turnaround_time += p.turnaround_time;
                total_waiting_time += p.waiting_time;
                completed_count++;

                completed_processes.push_back(p);
            }
        }
    }

    if (!gantt_chart.empty())
    {
        get<2>(gantt_chart.back()) = current_time;
    }

    float average_turnaround = total_turnaround_time / completed_processes.size();
    float average_waiting = total_waiting_time / completed_processes.size();

    printGanttChart(gantt_chart);

    printProcessMetrics(completed_processes, "Turnaround Time", average_turnaround);

    printProcessMetrics(completed_processes, "Waiting Time", average_waiting);
}

void getInput(vector<Process> &processes, bool askPriority)
{
    int num_processes;
    cout << "Enter the number of processes: ";
    cin >> num_processes;

    for (int i = 0; i < num_processes; ++i)
    {
        Process p;
        p.id = i + 1;
        cout << "Enter arrival time for Process " << p.id << ": ";
        cin >> p.arrival_time;
        cout << "Enter burst time for Process " << p.id << ": ";
        cin >> p.burst_time;

        if (askPriority)
        {
            cout << "Enter priority for Process " << p.id << " (lower value = higher priority): ";
            cin >> p.priority;
        }
        else
        {
            p.priority = 0; // Default value, not used for non-priority algorithms
        }

        p.remaining_time = p.burst_time; // Initialize remaining_time for SRT and RR
        processes.push_back(p);
    }
}

bool askForAnotherOperation()
{
    char choice;
    cout << "\nDo you want to perform another operation? (y/n): ";
    cin >> choice;
    return choice == 'y' || choice == 'Y';
}
int main()
{
    do
    {
        int choice;
        cout << "\nSelect Scheduling Algorithm:\n";
        cout << "1. Shortest Job Next (SJN)\n";
        cout << "2. Non-Preemptive Priority (NPP)\n";
        cout << "3. Round Robin (RR)\n";
        cout << "4. Shortest Remaining Time (SRT)\n";
        cin >> choice;

        vector<Process> processes;

        switch (choice)
        {
        case 1:
            cout << "You selected Shortest Job Next (SJN).\n";
            getInput(processes, false);
            sjn(processes, processes.size());
            break;
        case 2:
            cout << "You selected Non-Preemptive Priority (NPP).\n";
            getInput(processes, true); // Ask for priority
            nonPreemptivePriority(processes);
            break;
        case 3:
        {
            cout << "You selected Round Robin (RR).\n";
            getInput(processes, false);
            int quantum;
            cout << "Enter time quantum for Round Robin: ";
            cin >> quantum;
            roundRobin(processes, quantum);
            break;
        }
        case 4:
            cout << "You selected Shortest Remaining Time (SRT).\n";
            getInput(processes, false);
            srt(processes);
            break;
        default:
            cout << "Invalid choice!\n";
            continue; // Restart the loop if invalid
        }
    } while (askForAnotherOperation());

    cout << "Exiting program. Thank you!\n";
    return 0;
}