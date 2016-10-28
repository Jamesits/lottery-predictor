#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#define DATASET_BUFFER_LENGTH 16

// (3 - √5) / 2
#define MAGIC_NUMBER 0.38196601125

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
            d = (period *)realloc(d, (dataset_size += DATASET_BUFFER_LENGTH) * sizeof(period));
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
        if (readcount != 9) {           // entered malformed line
            fprintf(stderr, "encounted file read error at line %d", dataset_current_index);
            exit(EXIT_FAILURE);
        }
        ++dataset_current_index;
    }
    d[dataset_current_index].ballset = 0;   // end mark
    return d;
}

void predict(period *data, period *prediction) {
    // let's assume data is entered in date ascend sequence
    double probability_matrix[6][2] = {{1, 33}, {1, 33}, {1, 33}, {1, 33}, {1, 33}, {1, 33}};
    double average[6] = {0};
    double stddev[6] = {0};
    int max_index = 0;
    while(data[++max_index].ballset);
    
    for (int i = max_index - 1; i >= 0; --i) {
	    for (int j = 0; j < 6; ++j) {
		    average[j] += data[i].balls[j];
	    }
    }
    for (int i = 0; i < 6; ++i) average[i] /= max_index;
    
    for (int i = max_index - 1; i >= 0; --i) {
	    for (int j = 0; j < 6; ++j) {
		    stddev[j] += pow(data[i].balls[j] - average[j], 2);
	    }
    }
    for (int i = 0; i < 6; ++i) {
	    stddev[i] = sqrt(stddev[i] / (max_index - 1));
    }

    for (int i = max_index - 1; i >= 0; --i) { // foreach data in data descend (so more recent data will have more influence)
        for (int j = 0; j < 6; ++j) {   // foreach position
            // refine new border in probability_matrix[j]
            /*
                均衡器法：一种魔改的折纸法

                原理：
                    0. 所有数字的出现是随机分布的
                    1. 对任何数字 X，X 和临近 X 的两三个数字被选中的可能性都小于其余数字被选中的可能性之和
                    2. 如果某一次出现了数字 X，那么接下来一段时间内出现 X（以及临近 X 的几个数）的概率就会相对小一些

                方法：
                    0. 把有效范围 1-33 划分为三个部分：可能性较大的中间区域和可能性较小的左右两侧
                    |1--------------L---------------------------R--------------33|
                                    ^                           ^
                    1. L 初始值为 1，R 初始值为 33
                    2. 如果已有数据落在 L 和 R 之间，那么基于原理 1，让靠近它的端点向它所在的（相对高可能性区域中心的）反方向挪动一点
                    3. 如果已有数据里面出现在 L 左侧或者 R 右侧，说明目前的最大可能性范围有点小了，所以让靠近它的端点更靠近它一点

            */
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

    // avoid collision
    for (int i = 0; i < 6; ++i) {
        for (int j = i + 1; j < 6; ++j) {
		while (prediction->balls[i] == prediction->balls[j]) {
			int direction = rand() >= 0.5 ? -1 : 1;
			int times = rand() * 3;
			prediction->balls[j] = (int)(prediction->balls[j] + direction * times * stddev[j] - direction * 1) % 33;
			if (prediction->balls[j] < 0) prediction->balls[j] += 33;
			prediction->balls[j] += 1;
		}
	}
    }

    // print debug information
    printf("Predictor information\n");
    printf("ball\taverage\t\tstddev\t\trmin\t\trmax\t\trcenter\n");
    for (int i = 0; i < 6; ++i) {
	    printf("%d\t%lf\t%lf\t%lf\t%lf\t%d\n", i + 1, average[i], stddev[i], probability_matrix[i][0], probability_matrix[i][1], (int)((probability_matrix[i][0] + probability_matrix[i][1]) / 2));
    }
}

int main(void) {
    srand(time(NULL));
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
