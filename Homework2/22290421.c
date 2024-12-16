// Ali Berkay Görgülü | 22290421

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// Structure to hold data for each student
typedef struct {
    int id;                 // Student ID
    float average;          // Average grade of the student
    int passed;             // 1 if the student passed, 0 if failed
    int *grades;            // Array of grades for each question
} Student;

int N, M;                   // N = Number of students, M = Number of questions
int highest_grade = 0;      // Variable to store the highest grade
int lowest_grade = 100;     // Variable to store the lowest grade
int *passed_per_question;   // Array to track number of students passing each question
int total_passed = 0;       // Total number of students who passed
sem_t sem_stats;            // Semaphore for synchronizing updates to global statistics

Student *students;          // Array of students
pthread_t *student_threads; // Array of threads for each student

/**
 * Function to read input data from a file.
 * This function reads the number of students (N) and questions (M),
 * allocates memory for the students and their grades, and initializes
 * necessary global variables.
 * 
 * @param file_path The path of the input file.
 */
void readInput(const char *file_path);

/**
 * Function to write the output results to a file.
 * This function writes the student results (ID, average, pass/fail),
 * along with overall statistics such as the number of students passing
 * each question, the total number of students who passed, and the highest 
 * and lowest grades.
 * 
 * @param file_path The path of the output file.
 */
void writeOutput(const char *file_path);

/**
 * Thread function to process a student's grades.
 * This function calculates the student's average grade, determines if 
 * the student passed, and then updates global statistics using a semaphore
 * to ensure thread safety when modifying shared resources.
 * 
 * @param arg Pointer to the Student structure.
 * @return NULL to conform with pthread's requirements.
 */
void *studentThread(void *arg);

/**
 * Function to update global statistics based on a student's grades.
 * This function checks the student's grades and updates the following
 * global statistics:
 *  - Total number of students passing each question.
 *  - Total number of students who passed.
 *  - Highest and lowest grades across all students.
 * 
 * @param grades Pointer to the array of grades for a student.
 * @param average The average grade of the student.
 * @param passed Flag indicating whether the student passed (1) or failed (0).
 */
void postProcessStatistics(int *grades, float average, int passed);

/**
 * Function to clean up resources before the program exits.
 * This function frees allocated memory and destroys the semaphore.
 */
void quit();

int main() {
    readInput("input.txt");

    for (int i = 0; i < N; i++) {
        if (pthread_create(&student_threads[i], NULL, studentThread, &students[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    for (int i = 0; i < N; i++) {
        if (pthread_join(student_threads[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }
    }

    writeOutput("results.txt");

    quit();

    return 0;
}

void readInput(const char *file_path) {
    FILE *input = fopen(file_path, "r");
    if (!input) {
        perror("Failed to open input file");
        return;
    }

    if (fscanf(input, "%d %d", &N, &M) != 2) {
        perror("Failed to read N and M from input file");
        fclose(input);
        return;
    }

    students = malloc(N * sizeof(Student));
    passed_per_question = calloc(M, sizeof(int));
    student_threads = malloc(N * sizeof(pthread_t));
    sem_init(&sem_stats, 0, 1);

    for (int i = 0; i < N; i++) {
        students[i].grades = malloc(M * sizeof(int));
        if (fscanf(input, "%d", &students[i].id) != 1) {
            perror("Failed to read student ID");
            fclose(input);
            return;
        }
        for (int j = 0; j < M; j++) {
            if (fscanf(input, "%d", &students[i].grades[j]) != 1) {
                perror("Failed to read student grades");
                fclose(input);
                return;
            }
        }
    }
    
    fclose(input);
}

void writeOutput(const char *file_path) {
    FILE *output = fopen(file_path, "w");
    if (!output) {
        perror("Failed to open output file");
        return;
    }

    for (int i = 0; i < N; i++) {
        fprintf(output, "%d %.2f %s\n", students[i].id, students[i].average,
                students[i].passed ? "Passed" : "Failed");
    }

    fprintf(output, "\n");

    fprintf(output, "--- Overall Statistics ---\n");
    fprintf(output, "Number of students passing each question:\n");
    for (int j = 0; j < M; j++) { fprintf(output, "Question %d: %d students passed.\n", j + 1, passed_per_question[j]); }
    fprintf(output, "Total number of students who passed overall: %d\n", total_passed);
    fprintf(output, "Highest grade: %d\n", highest_grade);
    fprintf(output, "Lowest grade: %d\n", lowest_grade);

    fclose(output);
}

void *studentThread(void *arg) {
    Student *student = (Student *)arg;

    int sum = 0;
    for (int i = 0; i < M; i++) { sum += student->grades[i]; }
    student->average = sum / (float)M; // Calculate average grade
    student->passed = (student->average >= 60); //  Check for average passing requirement

    sem_wait(&sem_stats); // Wait while a thread manipulates the global variables
    postProcessStatistics(student->grades, student->average, student->passed);
    sem_post(&sem_stats); // Signal for other threads to continue

    return NULL;
}

void postProcessStatistics(int *grades, float average, int passed) {
    if (passed) total_passed++; // Check if a student has passed to increase the total passed count

    for (int i = 0; i < M; i++) {
        if (grades[i] >= 60) { passed_per_question[i]++; } //  Check for question passing requirement
        if (grades[i] > highest_grade) { highest_grade = grades[i]; } // Check if the grade is the highest
        if (grades[i] < lowest_grade) { lowest_grade = grades[i]; } // Check if the is the lowest
    }
}

void quit() {
    for (int i = 0; i < N; i++) { free(students[i].grades); }
    free(students);
    free(passed_per_question);
    free(student_threads);
    sem_destroy(&sem_stats);
}
