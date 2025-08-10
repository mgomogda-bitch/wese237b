#include "huffman.h"
#include <queue>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <cstring>

// Node for Huffman Tree
struct HuffmanNode {
    unsigned char data;
    unsigned freq;
    HuffmanNode *left, *right;

    HuffmanNode(unsigned char data, unsigned freq) : data(data), freq(freq), left(nullptr), right(nullptr) {}
};

// Comparator for priority queue
struct CompareNode {
    bool operator()(HuffmanNode* l, HuffmanNode* r) {
        return l->freq > r->freq;
    }
};

// Traverse Huffman Tree to build codes
typedef std::unordered_map<unsigned char, std::string> HuffmanCodeMap;
void buildCodeMap(HuffmanNode* root, std::string str, HuffmanCodeMap &huffmanCode) {
    if (!root) return;

    if (!root->left && !root->right) {
        huffmanCode[root->data] = str;
    }

    buildCodeMap(root->left, str + "0", huffmanCode);
    buildCodeMap(root->right, str + "1", huffmanCode);
}

// Free Huffman Tree memory
void freeTree(HuffmanNode* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

// Huffman Encode Implementation
int huffman_encode(const unsigned char *bufin, uint32_t bufinlen, unsigned char **pbufout, uint32_t *pbufoutlen) {
    if (bufin == nullptr || bufinlen == 0) return -1;

    // Count frequency
    std::unordered_map<unsigned char, unsigned> freq;
    for (uint32_t i = 0; i < bufinlen; ++i) {
        freq[bufin[i]]++;
    }

    // Priority queue for Huffman Tree
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareNode> pq;
    for (auto pair : freq) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    // Build Huffman Tree
    while (pq.size() > 1) {
        HuffmanNode *left = pq.top(); pq.pop();
        HuffmanNode *right = pq.top(); pq.pop();

        HuffmanNode *node = new HuffmanNode('\0', left->freq + right->freq);
        node->left = left;
        node->right = right;
        pq.push(node);
    }

    // Build Huffman codes
    HuffmanCodeMap huffmanCode;
    buildCodeMap(pq.top(), "", huffmanCode);

    // Encode input
    std::string encodedStr;
    for (uint32_t i = 0; i < bufinlen; ++i) {
        encodedStr += huffmanCode[bufin[i]];
    }

    // Padding to byte boundary
    int padding = 8 - encodedStr.size() % 8;
    encodedStr += std::string(padding, '0');

    // Allocate output
    *pbufoutlen = encodedStr.size() / 8 + 256;
    *pbufout = (unsigned char*)malloc(*pbufoutlen);
    memset(*pbufout, 0, *pbufoutlen);

    // Store codes for decoding
    int idx = 0;
    for (auto pair : huffmanCode) {
        (*pbufout)[idx++] = pair.first;
        (*pbufout)[idx++] = pair.second.size();
        for (char c : pair.second) {
            (*pbufout)[idx++] = c;
        }
    }
    (*pbufout)[idx++] = 0; // End of header

    // Store encoded data
    for (size_t i = 0; i < encodedStr.size(); i += 8) {
        std::bitset<8> bits(encodedStr.substr(i, 8));
        (*pbufout)[idx++] = (unsigned char)bits.to_ulong();
    }

    freeTree(pq.top());
    *pbufoutlen = idx;
    return 0;
}

// Huffman Decode Implementation
int huffman_decode(const unsigned char *bufin, uint32_t bufinlen, unsigned char **pbufout, uint32_t *pbufoutlen) {
    if (bufin == nullptr || bufinlen == 0) return -1;

    HuffmanCodeMap reverseCode;

    // Parse Huffman codes from header
    int idx = 0;
    while (bufin[idx] != 0) {
        unsigned char data = bufin[idx++];
        unsigned len = bufin[idx++];
        std::string code;
        for (unsigned i = 0; i < len; ++i) {
            code += bufin[idx++];
        }
        reverseCode[code] = data;
    }
    idx++;

    // Decode input
    std::string encodedStr;
    for (; idx < bufinlen; ++idx) {
        encodedStr += std::bitset<8>(bufin[idx]).to_string();
    }

    std::vector<unsigned char> decodedVec;
    std::string current;
    for (char bit : encodedStr) {
        current += bit;
        if (reverseCode.count(current)) {
            decodedVec.push_back(reverseCode[current]);
            current.clear();
        }
    }

    *pbufoutlen = decodedVec.size();
    *pbufout = (unsigned char*)malloc(*pbufoutlen + 1);
    memcpy(*pbufout, decodedVec.data(), *pbufoutlen);
    (*pbufout)[*pbufoutlen] = '\0';

    return 0;
}
