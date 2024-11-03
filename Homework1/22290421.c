// Ali Berkay Görgülü | 22290421

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_INVESTMENTS 100

/**
 * Function to calculate the Sharpe Ratio.
 * @param expected_return The expected return on investment (%).
 * @param standard_deviation The standard deviation of the investment (risk).
 * @param risk_free_rate The risk-free interest rate (%).
 * @return The calculated Sharpe Ratio.
 */
double calculate_sharpe_ratio(double expected_return, double standard_deviation, double risk_free_rate);

/**
 * Function for the child process.
 * @param read_fd File descriptor for reading investment data from the parent.
 * @param write_fd File descriptor for writing calculated Sharpe Ratios back to the parent.
 */
void child_process(int read_fd, int write_fd);

/**
 * Function for the parent process.
 * @param write_fd File descriptor for writing investment data to the child.
 * @param read_fd File descriptor for reading calculated Sharpe Ratios from the child.
 */
void parent_process(int write_fd, int read_fd);

int main() {
    int pipe1[2]; // Pipe for parent to child data flow
    int pipe2[2]; // Pipe for child to parent data flow

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);

        child_process(pipe1[0], pipe2[1]);
        exit(EXIT_SUCCESS);
    } else {
        close(pipe1[0]);
        close(pipe2[1]);

        parent_process(pipe1[1], pipe2[0]);
        wait(NULL);
    }

    return 0;
}

double calculate_sharpe_ratio(double expected_return, double standard_deviation, double risk_free_rate) {
    if (standard_deviation == 0) return 0.0; // Avoid division by zero
    return (expected_return - risk_free_rate) / standard_deviation;
}

void child_process(int read_fd, int write_fd) {
    double expected_return, risk_free_rate, standard_deviation;
    while (read(read_fd, &expected_return, sizeof(double)) > 0 &&
           read(read_fd, &standard_deviation, sizeof(double)) > 0 &&
           read(read_fd, &risk_free_rate, sizeof(double)) > 0) {
        
        double sharpe_ratio = calculate_sharpe_ratio(expected_return, standard_deviation, risk_free_rate);
        
        // Send the Sharpe Ratio back to the parent
        write(write_fd, &sharpe_ratio, sizeof(double));
    }
    close(read_fd);
    close(write_fd);
}

void parent_process(int write_fd, int read_fd) {
    double expected_return, risk_free_rate, standard_deviation;
    int investment_count = 0;
    double sharpe_ratios[MAX_INVESTMENTS];

    while (1) {
        if (scanf("%lf %lf %lf", &expected_return, &risk_free_rate, &standard_deviation) != 3) {
            char finish[10];
            scanf("%s", finish);
            if (strcmp(finish, "finish") == 0) {
                break;
            }
            fprintf(stderr, "Invalid input format. Please provide numeric values.\n");
            continue;
        }

        investment_count++;
        write(write_fd, &expected_return, sizeof(double));
        write(write_fd, &risk_free_rate, sizeof(double));
        write(write_fd, &standard_deviation, sizeof(double));
    }

    close(write_fd);

    // Read Sharpe Ratios from child process
    for (int i = 0; i < investment_count; i++) {
        double sharpe_ratio;
        read(read_fd, &sharpe_ratio, sizeof(double));
        sharpe_ratios[i] = sharpe_ratio;
    }

    close(read_fd);

    // Determine the best investment based on the highest Sharpe Ratio
    int best_investment_index = 0;
    for (int i = 1; i < investment_count; i++) {
        if (sharpe_ratios[i] > sharpe_ratios[best_investment_index]) {
            best_investment_index = i;
        }
    }

    for (int i = 0; i < investment_count; i++) {
        printf("%.2f\n", sharpe_ratios[i]);
    }

    printf("Selected Investment: %d", best_investment_index + 1);
}