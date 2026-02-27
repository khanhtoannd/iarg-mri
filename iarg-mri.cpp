#include <iostream>
#include <queue>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <windows.h>
#include <direct.h>
#include <iomanip>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

// ================= BIEN TOAN CUC =================
int rong, cao;
unsigned char* anhRGB = NULL;
int* anhXam = NULL;
int* anhBlur = NULL;
int* gradient = NULL;
unsigned char* brainMask = NULL;
unsigned char* mask = NULL;
bool* daTham = NULL;

// ================= TRUY CAP =================
int idx(int x, int y) {
    return y * rong + x;
}

// ================= GIAI PHONG =================
void giaiPhong() {
    if (anhRGB) stbi_image_free(anhRGB);
    if (anhXam) delete[] anhXam;
    if (anhBlur) delete[] anhBlur;
    if (gradient) delete[] gradient;
    if (brainMask) delete[] brainMask;
    if (mask) delete[] mask;
    if (daTham) delete[] daTham;

    anhRGB = NULL; anhXam = NULL; anhBlur = NULL;
    gradient = NULL; brainMask = NULL; mask = NULL; daTham = NULL;
}

// ================= DOC ANH =================
void docAnh(const char* f) {
    int c;
    anhRGB = stbi_load(f, &rong, &cao, &c, 3);
    if (!anhRGB) {
        cout << "Khong doc duoc anh: " << f << endl;
        exit(1);
    }
}

// ================= CHUYEN XAM =================
void chuyenXam() {
    anhXam = new int[rong * cao];
    for (int i = 0; i < rong * cao; i++)
        anhXam[i] = (anhRGB[i * 3] + anhRGB[i * 3 + 1] + anhRGB[i * 3 + 2]) / 3;
}

// ================= GAUSSIAN BLUR =================
void gaussianBlur() {
    anhBlur = new int[rong * cao];
    memset(anhBlur, 0, sizeof(int) * rong * cao);

    int k[3][3] = { {1,2,1},{2,4,2},{1,2,1} };

    for (int y = 1; y < cao - 1; y++)
        for (int x = 1; x < rong - 1; x++) {
            int s = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    s += anhXam[idx(x + dx, y + dy)] * k[dy + 1][dx + 1];
            anhBlur[idx(x, y)] = s / 16;
        }
}

// ================= GRADIENT =================
void tinhGradient() {
    gradient = new int[rong * cao];
    memset(gradient, 0, sizeof(int) * rong * cao);

    for (int y = 1; y < cao - 1; y++)
        for (int x = 1; x < rong - 1; x++)
            gradient[idx(x, y)] =
            abs(anhBlur[idx(x + 1, y)] - anhBlur[idx(x - 1, y)]) +
            abs(anhBlur[idx(x, y + 1)] - anhBlur[idx(x, y - 1)]);
}

// ================= BRAIN MASK =================
void taoBrainMask() {
    brainMask = new unsigned char[rong * cao];
    memset(brainMask, 0, rong * cao);

    for (int i = 0; i < rong * cao; i++)
        if (anhBlur[i] > 30) brainMask[i] = 255;

    queue<int> q;
    for (int x = 0; x < rong; x++) {
        q.push(idx(x, 0));
        q.push(idx(x, cao - 1));
    }
    for (int y = 0; y < cao; y++) {
        q.push(idx(0, y));
        q.push(idx(rong - 1, y));
    }

    int dx[4] = { -1,1,0,0 };
    int dy[4] = { 0,0,-1,1 };

    while (!q.empty()) {
        int p = q.front(); q.pop();
        if (!brainMask[p]) continue;
        brainMask[p] = 0;
        int x = p % rong, y = p / rong;
        for (int k = 0; k < 4; k++) {
            int xn = x + dx[k], yn = y + dy[k];
            if (xn >= 0 && xn < rong && yn >= 0 && yn < cao)
                q.push(idx(xn, yn));
        }
    }
}

// ================= TIM SEED =================
vector<int> timSeed() {
    vector< pair<int, int> > ds;
    int cx = rong / 2, cy = cao / 2;

    for (int y = 20; y < cao - 20; y++)
        for (int x = 20; x < rong - 20; x++) {
            int id = idx(x, y);
            if (!brainMask[id]) continue;

            int mean = 0, var = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    mean += anhBlur[idx(x + dx, y + dy)];
            mean /= 9;

            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int d = anhBlur[idx(x + dx, y + dy)] - mean;
                    var += d * d;
                }

            int score = mean * 2 - var - gradient[id] * 3
                - abs(x - cx) - abs(y - cy);

            if (score > 80)
                ds.push_back(make_pair(score, id));
        }

    sort(ds.begin(), ds.end());

    vector<int> seeds;
    int limit = ds.size() < 20 ? (int)ds.size() : 20;
    for (int i = 0; i < limit; i++)
        seeds.push_back(ds[ds.size() - 1 - i].second);

    return seeds;
}

// ================= REGION GROWING =================
void regionGrowing() {
    mask = new unsigned char[rong * cao];
    daTham = new bool[rong * cao];
    memset(mask, 0, rong * cao);
    memset(daTham, false, sizeof(bool) * rong * cao);

    vector<int> seeds = timSeed();
    int cx = rong / 2, cy = cao / 2;
    int maxR = (rong < cao ? rong : cao) * 35 / 100;
    int maxR2 = maxR * maxR;

    int dx[4] = { -1,1,0,0 };
    int dy[4] = { 0,0,-1,1 };

    for (int si = 0; si < (int)seeds.size(); si++) {
        int s = seeds[si];
        if (daTham[s]) continue;

        queue<int> q;
        q.push(s);
        daTham[s] = true;
        mask[s] = 255;

        int sum = anhBlur[s], cnt = 1;

        while (!q.empty()) {
            int p = q.front(); q.pop();
            int x = p % rong, y = p / rong;

            int mean = sum / cnt;
            int eps = mean / 4;
            if (eps < 18) eps = 18;
            if (eps > 45) eps = 45;

            for (int k = 0; k < 4; k++) {
                int xn = x + dx[k], yn = y + dy[k];
                if (xn < 0 || xn >= rong || yn < 0 || yn >= cao) continue;

                int dxx = xn - cx, dyy = yn - cy;
                if (dxx * dxx + dyy * dyy > maxR2) continue;

                int id = idx(xn, yn);
                if (daTham[id]) continue;
                if (!brainMask[id]) continue;

                int diff = abs(anhBlur[id] - mean);

                // SIET THEM: khong an vao bien manh
                if (diff <= eps & gradient[id] <= 60) {
                    daTham[id] = true;
                    mask[id] = 255;
                    sum += anhBlur[id];
                    cnt++;
                    q.push(id);
                }
            }
        }
    }
}

// ================= GIU VUNG GAN TAM (FIX CHUAN) =================
void giuVungGanTam() {
    int cx = rong / 2, cy = cao / 2;

    bool* visited = new bool[rong * cao];
    memset(visited, false, sizeof(bool) * rong * cao);

    unsigned char* bestComp = new unsigned char[rong * cao];
    memset(bestComp, 0, rong * cao);

    int dx[4] = { -1,1,0,0 };
    int dy[4] = { 0,0,-1,1 };

    int bestDist = 1000000000;

    for (int i = 0; i < rong * cao; i++) {
        if (mask[i] && !visited[i]) {
            queue<int> q;
            q.push(i);
            visited[i] = true;

            vector<int> comp;
            comp.push_back(i);

            int sumDist = 0, cnt = 0;

            while (!q.empty()) {
                int p = q.front(); q.pop();
                int x = p % rong, y = p / rong;

                sumDist += abs(x - cx) + abs(y - cy);
                cnt++;

                for (int k = 0; k < 4; k++) {
                    int xn = x + dx[k], yn = y + dy[k];
                    if (xn < 0 || xn >= rong || yn < 0 || yn >= cao) continue;

                    int id = idx(xn, yn);
                    if (mask[id] && !visited[id]) {
                        visited[id] = true;
                        q.push(id);
                        comp.push_back(id);
                    }
                }
            }

            int avgDist = sumDist / cnt;

            if (avgDist < bestDist) {
                bestDist = avgDist;

                memset(bestComp, 0, rong * cao);
                for (int t = 0; t < (int)comp.size(); t++)
                    bestComp[comp[t]] = 255;
            }
        }
    }

    memcpy(mask, bestComp, rong * cao);

    delete[] visited;
    delete[] bestComp;
}

// ================= MORPHOLOGY =================
void erosion(unsigned char* src, unsigned char* dst) {
    int dx[8] = { -1,0,1,-1,1,-1,0,1 };
    int dy[8] = { -1,-1,-1,0,0,1,1,1 };

    memset(dst, 0, rong * cao);
    for (int y = 1; y < cao - 1; y++)
        for (int x = 1; x < rong - 1; x++) {
            bool ok = true;
            for (int k = 0; k < 8; k++)
                if (!src[idx(x + dx[k], y + dy[k])]) ok = false;
            if (ok) dst[idx(x, y)] = 255;
        }
}

void dilation(unsigned char* src, unsigned char* dst) {
    int dx[8] = { -1,0,1,-1,1,-1,0,1 };
    int dy[8] = { -1,-1,-1,0,0,1,1,1 };

    memset(dst, 0, rong * cao);
    for (int y = 1; y < cao - 1; y++)
        for (int x = 1; x < rong - 1; x++)
            if (src[idx(x, y)])
                for (int k = 0; k < 8; k++)
                    dst[idx(x + dx[k], y + dy[k])] = 255;
}

void morphologySmooth() {
    unsigned char* t = new unsigned char[rong * cao];

    // closing: dilate -> erode (lam tron + bit lo nho)
    dilation(mask, t);
    erosion(t, mask);

    dilation(mask, t);
    erosion(t, mask);

    delete[] t;
}

// ================= FILL HOLES =================
void fillHoles() {
    unsigned char* inv = new unsigned char[rong * cao];
    unsigned char* mark = new unsigned char[rong * cao];
    memset(mark, 0, rong * cao);

    for (int i = 0; i < rong * cao; i++)
        inv[i] = mask[i] ? 0 : 255;

    queue<int> q;

    for (int x = 0; x < rong; x++) {
        if (inv[idx(x, 0)]) { mark[idx(x, 0)] = 255; q.push(idx(x, 0)); }
        if (inv[idx(x, cao - 1)]) { mark[idx(x, cao - 1)] = 255; q.push(idx(x, cao - 1)); }
    }
    for (int y = 0; y < cao; y++) {
        if (inv[idx(0, y)]) { mark[idx(0, y)] = 255; q.push(idx(0, y)); }
        if (inv[idx(rong - 1, y)]) { mark[idx(rong - 1, y)] = 255; q.push(idx(rong - 1, y)); }
    }

    int dx[4] = { -1,1,0,0 };
    int dy[4] = { 0,0,-1,1 };

    while (!q.empty()) {
        int p = q.front(); q.pop();
        int x = p % rong, y = p / rong;

        for (int k = 0; k < 4; k++) {
            int xn = x + dx[k], yn = y + dy[k];
            if (xn < 0 || xn >= rong || yn < 0 || yn >= cao) continue;
            int id = idx(xn, yn);
            if (inv[id] && !mark[id]) {
                mark[id] = 255;
                q.push(id);
            }
        }
    }

    for (int i = 0; i < rong * cao; i++) {
        if (inv[i] && !mark[i]) mask[i] = 255;
    }

    delete[] inv;
    delete[] mark;
}

// ================= GHI =================
void ghiMask(const char* f) {
    unsigned char* out = new unsigned char[rong * cao * 3];
    for (int i = 0; i < rong * cao; i++)
        out[i * 3] = out[i * 3 + 1] = out[i * 3 + 2] = mask[i];
    stbi_write_png(f, rong, cao, 3, out, rong * 3);
    delete[] out;
}

// ================= XU LY 1 ANH =================
void xuLyMotAnh(const char* in, const char* out) {
    docAnh(in);
    chuyenXam();
    gaussianBlur();
    tinhGradient();
    taoBrainMask();
    regionGrowing();
    giuVungGanTam();
    morphologySmooth();
    fillHoles();
    ghiMask(out);
    giaiPhong();
}

// ====================== DANH GIA THUC NGHIEM ======================
unsigned char* docMask1Kenh(const char* f, int& w, int& h) {
    int c;
    unsigned char* img = stbi_load(f, &w, &h, &c, 1);
    return img;
}

void tinhChiSo(unsigned char* pred, unsigned char* gt, int n,
    long long& TP, long long& FP, long long& FN, long long& TN) {

    TP = FP = FN = TN = 0;
    for (int i = 0; i < n; i++) {
        int p = pred[i] > 127 ? 1 : 0;
        int g = gt[i] > 127 ? 1 : 0;

        if (p == 1 && g == 1) TP++;
        else if (p == 1 && g == 0) FP++;
        else if (p == 0 && g == 1) FN++;
        else TN++;
    }
}

double diceScore(long long TP, long long FP, long long FN) {
    double denom = (2.0 * TP + FP + FN);
    if (denom == 0) return 1.0;
    return (2.0 * TP) / denom;
}

double iouScore(long long TP, long long FP, long long FN) {
    double denom = (1.0 * TP + FP + FN);
    if (denom == 0) return 1.0;
    return (1.0 * TP) / denom;
}

double precisionScore(long long TP, long long FP) {
    double denom = (1.0 * TP + FP);
    if (denom == 0) return 1.0;
    return (1.0 * TP) / denom;
}

double recallScore(long long TP, long long FN) {
    double denom = (1.0 * TP + FN);
    if (denom == 0) return 1.0;
    return (1.0 * TP) / denom;
}

void danhGiaThuMuc(const char* thuMucGT, const char* thuMucPred, const char* fileCSV) {
    ofstream fo(fileCSV);
//    fo << "file,dice,iou,precision,recall\n";

    // ====== IN BANG CONSOLE ======
    fo << "\n==================== BANG DANH GIA MASK ====================\n";
    fo << string(84, '-') << endl;
    fo << left
         <<"| "<< setw(25) << "File"
         <<"| "<< setw(12) << "Dice"
         <<"| "<< setw(12) << "IoU"
         <<"| "<< setw(12) << "Precision"
         <<"| "<< setw(12) << "Recall"
         <<"| "<< endl;

    fo << string(84, '-') << endl;

    char pattern[300];
    strcpy(pattern, thuMucGT);
    strcat(pattern, "\\*.*");

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(pattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        cout << "Khong mo duoc thu muc mask chuan!\n";
        return;
    }

    double sumDice = 0, sumIoU = 0, sumP = 0, sumR = 0;
    int dem = 0;

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        char* tenFile = ffd.cFileName;
        char* dot = strrchr(tenFile, '.');
        if (!dot) continue;

        if (_stricmp(dot, ".png") != 0 &&
            _stricmp(dot, ".jpg") != 0 &&
            _stricmp(dot, ".jpeg") != 0) continue;

        char gtPath[400], predPath[400];

        // ===== GT PATH =====
        strcpy(gtPath, thuMucGT);
        strcat(gtPath, "\\");
        strcat(gtPath, tenFile);

        // ===== PRED PATH: output/<stem>_mask.png =====
        strcpy(predPath, thuMucPred);
        strcat(predPath, "\\");

        char stem[300];
        strcpy(stem, tenFile);
        char* dot2 = strrchr(stem, '.');
        if (dot2) *dot2 = 0;

        strcat(predPath, stem);
        strcat(predPath, "_mask.png");

        int w1, h1, w2, h2;
        unsigned char* gt = docMask1Kenh(gtPath, w1, h1);
        unsigned char* pred = docMask1Kenh(predPath, w2, h2);

        if (!gt || !pred || w1 != w2 || h1 != h2) {
            cout << left << setw(25) << tenFile
                << "SKIP (khong doc duoc/khac size)\n";

            if (gt) stbi_image_free(gt);
            if (pred) stbi_image_free(pred);
            continue;
        }

        long long TP, FP, FN, TN;
        tinhChiSo(pred, gt, w1 * h1, TP, FP, FN, TN);

        double dice = diceScore(TP, FP, FN);
        double iou = iouScore(TP, FP, FN);
        double pre = precisionScore(TP, FP);
        double rec = recallScore(TP, FN);

        // ===== IN RA BANG =====
        fo <<"| "<< left << setw(25) << tenFile
             << fixed << setprecision(4)
             <<"| "<< setw(12) << dice
             <<"| "<< setw(12) << iou
             <<"| "<< setw(12) << pre
             <<"| "<< setw(12) << rec
             <<"| "<< endl;
        

        // ===== GHI CSV =====
//        fo << tenFile << "," << dice << "," << iou << "," << pre << "," << rec << "\n";

        sumDice += dice;
        sumIoU += iou;
        sumP += pre;
        sumR += rec;
        dem++;

        stbi_image_free(gt);
        stbi_image_free(pred);

    } while (FindNextFileA(hFind, &ffd));

    FindClose(hFind);

    fo << string(84, '-') << endl;

    if (dem > 0) {
        double meanDice = sumDice / dem;
        double meanIoU = sumIoU / dem;
        double meanP = sumP / dem;
        double meanR = sumR / dem;

        fo <<"| "<< left << setw(25) << "MEAN"
             << fixed << setprecision(4)
             <<"| "<< setw(12) << meanDice
             <<"| "<< setw(12) << meanIoU
             <<"| "<< setw(12) << meanP
             <<"| "<< setw(12) << meanR
             <<"| "<< endl;

//        fo << "MEAN," << meanDice << "," << meanIoU << "," << meanP << "," << meanR << "\n";
    }

//    cout << "============================================================\n";
	fo << string(84, '=') << endl;
    fo.close();

    cout << "Da xuat file CSV: " << fileCSV << endl;
}


// ================= MAIN =================
int main() {
    const char* thuMucVao = "input";
    const char* thuMucRa = "output";

    _mkdir(thuMucRa); // neu ton tai roi thi cung khong sao

    char pattern[300];
    strcpy(pattern, thuMucVao);
    strcat(pattern, "\\*.*");

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(pattern, &ffd);

    if (hFind == INVALID_HANDLE_VALUE) {
        cout << "Khong mo duoc thu muc input!\n";
        return 0;
    }

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        char* tenFile = ffd.cFileName;

        char* dot = strrchr(tenFile, '.');
        if (!dot) continue;

        if (_stricmp(dot, ".png") != 0 &&
            _stricmp(dot, ".jpg") != 0 &&
            _stricmp(dot, ".jpeg") != 0) continue;

        char in[400];
        char out[400];

        strcpy(in, thuMucVao);
        strcat(in, "\\");
        strcat(in, tenFile);

        strcpy(out, thuMucRa);
        strcat(out, "\\");
        strcat(out, tenFile);

        char* dotOut = strrchr(out, '.');
        if (dotOut) strcpy(dotOut, "_mask.png");
        else strcat(out, "_mask.png");

        cout << "Dang xu ly: " << in << endl;
        xuLyMotAnh(in, out);

    } while (FindNextFileA(hFind, &ffd));

    FindClose(hFind);

    cout << "HOAN THANH TAO MASK!\n";

    // ======= DANH GIA THUC NGHIEM =======
    danhGiaThuMuc("maskchuan", "output", "ketqua1.txt");

    cout << "HOAN THANH DANH GIA!\n";
    return 0;
}

