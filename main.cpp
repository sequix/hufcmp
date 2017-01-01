/*
 * Author: sequix
 * Date: 2016/12/31
 * Filename: main.cpp
 * Descruption: Use huffman tree to compress/decompress a file.
 */
#include <unistd.h>
#include <bits/stdc++.h>
using namespace std;
#define NDEBUG

#define EXIT_FAIL_OP   1   // return code on wrong option
#define EXIT_FAIL_IO   2   // return code on IO error
#define EXIT_FAIL_FE   3   // return code on illegal file

typedef uint8_t  Byte;
typedef uint8_t  Bit;
typedef uint32_t Size;

const Size MAX_SIZE = (1LL << 32) - 1;

char *progname = NULL;
Size inputFileSize = 0;

void outputHelp();
void compress();
void decompress();

int main(int argc, char *argv[])
{
    char op;
    char *inputFilename = NULL;
    char *outputFilename = NULL;
    bool toDecompress = false;

    progname = argv[0];

    if(argc < 2) {
        outputHelp();
        exit(EXIT_FAIL_OP);
    }

    while((op = getopt(argc, argv, "-hdo:")) != -1) {
        switch(op) {
            case 1:
                inputFilename = optarg;
                break;
            case 'd':
                toDecompress = true;
                break;
            case 'h':
                outputHelp();
                exit(EXIT_FAIL_OP);
                break;
            case 'o':
                outputFilename = optarg;
                break;
        }
    }

    if(inputFilename == NULL) {
        fprintf(stderr, "%s: must specify input file\n", progname);
        exit(EXIT_FAIL_OP);
    }

    if(freopen(inputFilename, "rb", stdin) == NULL) {
        perror(progname);
        exit(EXIT_FAIL_IO);
    }

    if(fseek(stdin, 0L, SEEK_END) < 0) {
        perror(progname);
        exit(EXIT_FAIL_IO);
    } else if((inputFileSize = ftell(stdin)) >= MAX_SIZE && !toDecompress) {
        fprintf(stderr, "%s: too big to compress\n");
        exit(EXIT_FAIL_FE);
    }
    rewind(stdin);

    if(outputFilename != NULL) {
        if(freopen(outputFilename, "wb", stdout) == NULL) {
            perror(progname);
            exit(EXIT_FAIL_IO);
        }
    } else {
        if(freopen("/dev/stdout", "wb", stdout) == NULL) {
            perror(progname);
            exit(EXIT_FAIL_IO);
        }
    }

    if(toDecompress)
        decompress();
    else
        compress();
    exit(EXIT_SUCCESS);
}

void outputHelp()
{
    puts("usage: hufcmp [-h] [-d] [-o FILE] FILE");
    puts("");
    puts("-h\tshow this help message");
    puts("-d\tdecompress FILE, by default, compress");
    puts("-o FILE\tspecify output file, by default, output to stdout");
    puts("FILE\tspecify input file");
}

void countBytes(Size *cnt);
void huffmanEncode(Size *cnt);
void huffmanDecode();
void rleEncode(Size *cnt);
void rleDecode();

// count number of each byte's appearance
// decide whether use Huffman or RLE
void compress()
{
    int nm1 = 0;    // number of bytes which appear more than once
    Size cnt[256] = {0};

    countBytes(cnt);
    for(int i = 0; i < 256; ++i)
        if(cnt[i] > 0) ++nm1;

    // only when there are more than 1 kind byte, can we use huffman
    if(nm1 > 1)
        huffmanEncode(cnt);
    else if(nm1 == 1)
        rleEncode(cnt);
    else if(nm1 == 0)
        return;
}

// huffman tree node
struct Node {
    Byte byte;
    Node *left, *right;
    Node(Byte b, Node *l=NULL, Node *r=NULL): byte(b), left(l), right(r) {}
};

#ifndef NDEBUG
void logPrintTree(Node *p)
{
    if(p == NULL) {
        fprintf(stderr, "# ");
        return;
    }
    fprintf(stderr, "%d ", p->byte);
    logPrintTree(p->left);
    logPrintTree(p->right);
}

void logPrintTable(string *table)
{
    for(int i = 0; i < 256; ++i)
        fprintf(stderr, "%d: %s\n", i, table[i].c_str());
}
#endif // NDEBUG

// read huffman tree from stdin, use the tree to decode rest bits, and output
void decompress()
{
    int b = getchar();

    if(b == 'H') {
        huffmanDecode();
    } else if(b == 'R') {
        rleDecode();
    } else if(b == EOF) {
        return;
    } else {
        fprintf(stderr, "%s: invalid file type\n", progname);
        exit(EXIT_FAIL_FE);
    }
}

// count number of appearence of each byte
void countBytes(Size *cnt)
{
    int b;
    while((b = getchar()) != EOF)
        ++cnt[b];
    rewind(stdin);
}

// encode bytes from stdin with RLE
void rleEncode(Size *cnt)
{
    putchar('R');   // represent this is encoded with RLE
    for(int i = 0; i < 256; ++i) {
        if(cnt[i] > 0) {
            fwrite(&cnt[i], sizeof(Size), 1, stdout);
            putchar(i);
            break;
        }
    }
}

// use RLE to decode bytes from stdin
void rleDecode()
{
    Byte b;
    Size cnt = 0;

    fread(&cnt, sizeof(Size), 1, stdin);
    b = getchar();
    for(Size i = 0; i < cnt; ++i)
        putchar(b);
}

Node *buildTree(Size *cnt);
void buildTable(Node *p, string *table, string cur);
Node *readTree();
void writeTree(Node *root);
void hufEncodeFile(string *encodingTable);
void hufDecodeFile(Node *root, Size origSize);

// encode bytes from stdin with Huffman coding
void huffmanEncode(Size *cnt)
{
    string encodingTable[256];

    putchar('H');   // represent this is encoded with huffman tree
    fwrite(&inputFileSize, sizeof(Size), 1, stdout);

    Node *root = buildTree(cnt);
    writeTree(root);
    buildTable(root, encodingTable, "");
    hufEncodeFile(encodingTable);

#ifndef NDEBUG
    logPrintTree(root);
    fputc('\n', stderr);
    logPrintTable(encodingTable);
#endif // NDEBUG
}

// decode with huffman tree read from stdin
void huffmanDecode()
{
    Size origSize;
    fread(&origSize, sizeof(Size), 1, stdin);
    Node *root = readTree();
#ifndef NDEBUG
    logPrintTree(root);
#endif // NDEBUG
    hufDecodeFile(root, origSize);
}

// read a byte to buffer, return it bit by bit
// return EOF when reach it
int readBit()
{
    static int buf = 0;
    static int bufp = 8;

    if(bufp == 8) {
        if((buf = getchar()) == EOF)
            return buf;
        bufp = 0;
    }
    return (buf >> (bufp++)) & 1;
}

// wirte preorder traversal sequence to stdout
// every two bytes represents a Node
void writeTree(Node *p)
{
    if(p == NULL) { // 0000 0001 0000 0000 represents NULL
        putchar(0);
        putchar(1);
        return;
    }
    putchar(p->byte);
    putchar(0);
    writeTree(p->left);
    writeTree(p->right);
}

// build a huffman tree, return root of the tree
Node *buildTree(Size *cnt)
{
    int b;
    typedef pair<Size, Node*> P;  // number fo apperance, huffman node
    priority_queue<P, vector<P>, greater<P> > que;

    for(int i = 0; i < 256; ++i)
        if(cnt[i] > 0)
            que.push(P(cnt[i], new Node(i)));
    while(que.size() > 1) {
        P p1 = que.top(); que.pop();
        P p2 = que.top(); que.pop();
        Node *np = new Node(-1, p1.second, p2.second);
        que.push(P(p1.first + p2.first, np));
    }
    return que.top().second;
}

// build encoding table based on a huffman tree
void buildTable(Node *p, string *table, string cur)
{
    if(p == NULL) return;
    if(p->left == NULL && p->right == NULL) {
        table[p->byte] = cur;
        return;
    }
    buildTable(p->left, table, cur + '0');
    buildTable(p->right, table, cur + '1');
}

// write a bit into buffer, until buffer is full, write buffer out
void writeBit(Bit b, bool flush=false)
{
    static int buf = 0;
    static int bufp = 0;

    if(flush) {
        putchar(buf);
        buf = bufp = 0;
        return;
    }
    if(bufp == 8) {
        putchar(buf);
        buf = bufp = 0;
    }
    buf |= (b & 1) << bufp++;
}

// encode bytes from stdin with encodingTable
void hufEncodeFile(string *encodingTable)
{
    int b;
    while((b = getchar()) != EOF)
        for(int i = 0; encodingTable[b][i]; ++i)
            writeBit(encodingTable[b][i] - '0');
    writeBit(-1, true);
}

// use huffman tree to decode bytes from stdin
void hufDecodeFile(Node *root, Size origSize)
{
    int b;
    Size cnt = 0;
    Node *p = root;

    while((b = readBit()) != EOF) {
        p = (b == 0) ? p->left : p->right;
        if(p->left == NULL && p->right == NULL) {
            putchar(p->byte);
            if(++cnt == origSize) break;
            p = root;
        }
    }
}

// read preorder traversal sequence of a huffman tree
Node *readTree()
{
    Byte lowbit = getchar();
    Byte hghbit = getchar();
    if(lowbit == 0 && hghbit == 1)
        return NULL;
    Node *p = new Node(lowbit);
    p->left = readTree();
    p->right = readTree();
    return p;
}
