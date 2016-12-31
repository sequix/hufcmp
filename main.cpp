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

#define EXIT_FAIL_OPT   1   // return code on wrong option
#define EXIT_FAIL_IO    2   // return code on IO error

typedef unsigned char Byte;
typedef unsigned char Bit;

void outputHelp();
void compress();
void decompress();

int main(int argc, char *argv[])
{
    char op;
    char *inputFilename = NULL;
    char *outputFilename = NULL;
    bool toDecompress = false;

    if(argc < 2) {
        outputHelp();
        exit(EXIT_FAIL_OPT);
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
                exit(EXIT_FAIL_OPT);
                break;
            case 'o':
                outputFilename = optarg;
                break;
        }
    }

    if(inputFilename == NULL) {
        fprintf(stderr, "%s: must specify input file\n", argv[0]);
        exit(EXIT_FAIL_OPT);
    }

    if(freopen(inputFilename, "rb", stdin) == NULL) {
        perror(argv[0]);
        exit(EXIT_FAIL_IO);
    }

    if(outputFilename != NULL) {
        if(freopen(outputFilename, "wb", stdout) == NULL) {
            perror(argv[0]);
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

// huffman tree node
struct Node {
    Byte byte;
    Node *left, *right;
    Node(Byte b, Node *l=NULL, Node *r=NULL): byte(b), left(l), right(r) {}
};

// buildTree huffman tree, return root of the tree
Node *buildTree()
{
    int b;
    size_t cnt[256] = {0};
    typedef pair<size_t, Node*> P;  // number fo apperance, huffman node
    priority_queue<P, vector<P>, greater<P> > que;

    while((b = getchar()) != EOF)
        ++cnt[b];
    for(int i = 0; i < 256; ++i)
        if(cnt[i] > 0)
            que.push(P(cnt[i], new Node(i)));
// what if the file has only 1 node, fix it !!!!
    while(que.size() > 1) {
        P p1 = que.top(); que.pop();
        P p2 = que.top(); que.pop();
        Node *np = new Node(-1, p1.second, p2.second);
        que.push(P(p1.first + p2.first, np));
    }
    return que.top().second;
}

// wirte preorder traversal sequence to stdout
// every two bytes represents a Node
void writeTree(Node *p)
{
    if(p == NULL) {
        putchar(0);
        putchar(0);
        return;
    }
    putchar(p->byte);
    putchar(0);
    writeTree(p->left);
    writeTree(p->right);
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

// encode bytes from stdin with encodingTable
void encodeFile(string *encodingTable)
{
    int b;
    while((b = getchar()) != EOF)
        for(int i = 0; encodingTable[b][i]; ++i)
            writeBit(encodingTable[b][i] - '0');
    writeBit(-1, true);
}

// build huffman tree, encode file read from stdin, and output to stdout
void compress()
{
    string encodingTable[256];
    Node *root = buildTree();
    writeTree(root);
    buildTable(root, encodingTable, "");

#ifndef NDEBUG
    for(int i = 0; i < 256; ++i) {
        if(encodingTable[i] != "")
            fprintf(stderr, "%c: %s\n", i, encodingTable[i].c_str());
    }
#endif // NDEBUG

    fseek(stdin, 0L, SEEK_SET);
    encodeFile(encodingTable);
}

// read preorder traversal sequence of a huffman tree
Node *readTree()
{
    Byte lowbit = getchar();
    Byte hghbit = getchar();
    if(lowbit == 0 && hghbit == 0)
        return NULL;
    Node *p = new Node(lowbit);
    p->left = readTree();
    p->right = readTree();
    return p;
}

#ifndef NDEBUG
void printTree(Node *p)
{
    if(!p) { fprintf(stderr, "# "); return; }
    fprintf(stderr, "(%d %c) ", p->byte, p->byte);
    printTree(p->left);
    printTree(p->right);
}
#endif // NDEBUG

// use huffman tree to decode
void decodeFile(Node *root)
{
    int b;
    Node *p = root;

// this write few bits more, fix it
    while((b = readBit()) != EOF) {
        p = (b == 0) ? p->left : p->right;
        if(p->left == NULL && p->right == NULL) {
            putchar(p->byte);
            p = root;
        }
    }
}

// read huffman tree from stdin, use the tree to decode rest bits, and output
void decompress()
{
    Node *root = readTree();
#ifndef NDEBUG
    printTree(root);
    putchar('\n');
#endif // NDEBUG
    decodeFile(root);
}
