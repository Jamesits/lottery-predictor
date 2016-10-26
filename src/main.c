#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define DATASET_BUFFER_LENGTH 16
#define MAGIC_NUMBER 0.381966011

typedef struct period {
    char period[8];
    int ballset;
    int balls[7];
} period;

period *read(FILE *src)
{
    if (!src) return NULL;  // file read error
    period *d = NULL;
    int dataset_size = 0;
    int dataset_current_index = 0;
    while (true) {
        // expand dataset if needed
        if (dataset_size == dataset_current_index) {
            d = realloc(d, dataset_size += DATASET_BUFFER_LENGTH * sizeof(period));
            if (!d) {
                fprintf(stderr, "memory allocation failed\n");
                exit(EXIT_FAILURE);
            }
        }
        // get data
        period *current = d + dataset_current_index;
        int readcount = fscanf(src,
            "%7s%d%d%d%d%d%d%d%d",
            current->period,
            &current->ballset,
            current->balls + 0,
            current->balls + 1,
            current->balls + 2,
            current->balls + 3,
            current->balls + 4,
            current->balls + 5,
            current->balls + 6
        );
        if (readcount == EOF) break;    // reached file end
        if (readcount != 9) {
            fprintf(stderr, "encounted file read error at line %d", dataset_current_index);
            continue;   // entered malformed line
        }
        ++dataset_current_index;
    }
    d[dataset_current_index].ballset = 0;   // end mark
    return d;
}

void predict(period *data, period *prediction) {
    // let's assume data is entered in date ascend sequence
    double probability_matrix[6][2] = {{1, 33}, {1, 33}, {1, 33}, {1, 33}, {1, 33}, {1, 33}};
    int max_index = 0;
    while(data[++max_index].ballset);
    for (int i = max_index - 1; i >= 0; --i) { // foreach data in data desc
        for (int j = 0; j < 6; ++j) {   // foreach position
            // refine new border in probability_matrix[j]
            double segment_length = probability_matrix[j][1] - probability_matrix[j][0];
            double cut_length = segment_length * MAGIC_NUMBER;

            double segment_center = (probability_matrix[j][1] + probability_matrix[j][0]) / 2;

            bool preference = data[i].balls[j] > segment_center;    // 0: left; 1: right;

            if (data[i].balls[j] < probability_matrix[j][0] || data[i].balls[j] > probability_matrix[j][1]) {
                // if already fall out of border then extend border
                if (!preference) {  // fall into left side
                    probability_matrix[j][0] -= cut_length;
                } else {            // fall into right side
                    probability_matrix[j][1] += cut_length;
                }
            } else {
                // if fall inside border then shrink border
                if (!preference) {  // fall into left side
                    probability_matrix[j][0] += cut_length;
                } else {            // fall into right side
                    probability_matrix[j][1] -= cut_length;
                }
            }

        }
    }
    for (int i = 0; i < 6; ++i) {
        prediction->balls[i] = round((probability_matrix[i][0] + probability_matrix[i][1]) / 2);
        if (prediction->balls[i] < 1) prediction->balls[i] = 1;
        if (prediction->balls[i] > 33) prediction->balls[i] = 33;
    }
}

int main(void) {
    // open file
    FILE *input;
    input = fopen("./input.txt", "r");
    if (!input) {
        fprintf(stderr, "file open error\n");
        exit(EXIT_FAILURE);
    }

    // read data
    period *d = read(input);

    // do prediction
    period p;
    predict(d, &p);

    // print prediction
    printf("Prediction:");
    for (int i = 0; i < 6; ++i) printf(" %d", p.balls[i]);
    printf("\n");

    // cleanup
    fclose(input);
    free(d);

    return EXIT_SUCCESS;
}
