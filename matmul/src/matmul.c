#define N 50

int a[N][N];
int b[N][N];
int c[N][N];

int main() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            c[i][j] = 0;
            a[i][j] = i + j;
            b[i][j] = i - j;
        }
    }
    // traverse b
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int temp = b[i][j];
            b[i][j] = b[j][i];
            b[j][i] = temp;
        }
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                c[i][j] += a[i][k] * b[j][k];
            }
        }
    }
    return 0;
}