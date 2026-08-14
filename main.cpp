#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

using namespace std;

// ============================================================
// B+ Tree Implementation
// ============================================================

const int ORDER = 128;

const char* DATA_FILE = "data.bpt";

struct BPTreeNode {
    bool is_leaf;
    int num_keys;
    string keys[ORDER];
    int values[ORDER];
    BPTreeNode* children[ORDER];
    BPTreeNode* next;
    BPTreeNode* prev;
    BPTreeNode* parent;

    BPTreeNode(bool leaf) : is_leaf(leaf), num_keys(0), next(nullptr), prev(nullptr), parent(nullptr) {
        for (int i = 0; i < ORDER; i++) {
            children[i] = nullptr;
        }
    }
};

class BPTree {
private:
    BPTreeNode* root;

    BPTreeNode* find_leaf(const string& key) const {
        BPTreeNode* cur = root;
        while (!cur->is_leaf) {
            int i = 0;
            while (i < cur->num_keys && key > cur->keys[i]) {
                i++;
            }
            cur = cur->children[i];
        }
        return cur;
    }

    void insert_into_leaf(BPTreeNode* leaf, const string& key, int value) {
        int pos = 0;
        while (pos < leaf->num_keys) {
            if (leaf->keys[pos] > key || (leaf->keys[pos] == key && leaf->values[pos] > value)) {
                break;
            }
            pos++;
        }

        for (int i = leaf->num_keys; i > pos; i--) {
            leaf->keys[i] = leaf->keys[i-1];
            leaf->values[i] = leaf->values[i-1];
        }
        leaf->keys[pos] = key;
        leaf->values[pos] = value;
        leaf->num_keys++;
    }

    void insert_into_parent(BPTreeNode* old_child, const string& key, BPTreeNode* new_child) {
        BPTreeNode* parent = old_child->parent;

        if (parent == nullptr) {
            BPTreeNode* new_root = new BPTreeNode(false);
            new_root->keys[0] = key;
            new_root->children[0] = old_child;
            new_root->children[1] = new_child;
            new_root->num_keys = 1;
            old_child->parent = new_root;
            new_child->parent = new_root;
            root = new_root;
            return;
        }

        int pos = 0;
        while (pos < parent->num_keys && parent->children[pos] != old_child) {
            pos++;
        }

        for (int i = parent->num_keys; i > pos; i--) {
            parent->keys[i] = parent->keys[i-1];
        }
        for (int i = parent->num_keys + 1; i > pos + 1; i--) {
            parent->children[i] = parent->children[i-1];
        }
        parent->keys[pos] = key;
        parent->children[pos+1] = new_child;
        new_child->parent = parent;
        parent->num_keys++;

        if (parent->num_keys >= ORDER) {
            split_internal(parent);
        }
    }

    void split_internal(BPTreeNode* node) {
        int mid = ORDER / 2;
        string promote_key = node->keys[mid];

        BPTreeNode* new_node = new BPTreeNode(false);
        new_node->num_keys = node->num_keys - mid - 1;

        for (int i = 0; i < new_node->num_keys; i++) {
            new_node->keys[i] = node->keys[mid + 1 + i];
            new_node->children[i] = node->children[mid + 1 + i];
            new_node->children[i]->parent = new_node;
        }
        new_node->children[new_node->num_keys] = node->children[node->num_keys];
        new_node->children[new_node->num_keys]->parent = new_node;

        node->num_keys = mid;

        insert_into_parent(node, promote_key, new_node);
    }

    void split_leaf(BPTreeNode* leaf) {
        int mid = ORDER / 2;

        // Adjust mid to avoid splitting equal keys
        while (mid > 0 && leaf->keys[mid - 1] == leaf->keys[mid]) {
            mid--;
        }
        if (mid == 0) {
            mid = ORDER / 2;
        }

        BPTreeNode* new_leaf = new BPTreeNode(true);
        new_leaf->num_keys = leaf->num_keys - mid;

        for (int i = 0; i < new_leaf->num_keys; i++) {
            new_leaf->keys[i] = leaf->keys[mid + i];
            new_leaf->values[i] = leaf->values[mid + i];
        }

        leaf->num_keys = mid;

        string promote_key = leaf->keys[leaf->num_keys - 1];

        new_leaf->next = leaf->next;
        new_leaf->prev = leaf;
        if (leaf->next) {
            leaf->next->prev = new_leaf;
        }
        leaf->next = new_leaf;

        insert_into_parent(leaf, promote_key, new_leaf);
    }

public:
    BPTree() : root(nullptr) {
        root = new BPTreeNode(true);
    }

    ~BPTree() {
        free_tree(root);
    }

    void free_tree(BPTreeNode* node) {
        if (!node) return;
        if (!node->is_leaf) {
            for (int i = 0; i <= node->num_keys; i++) {
                free_tree(node->children[i]);
            }
        }
        delete node;
    }

    void insert(const string& key, int value) {
        BPTreeNode* leaf = find_leaf(key);

        // Find correct leaf: advance while the next leaf also has this key
        // and our value is larger than the max value in the current leaf
        BPTreeNode* cur = leaf;
        while (cur->num_keys > 0 && 
               cur->keys[0] == key && cur->keys[cur->num_keys - 1] == key && 
               cur->next && cur->next->num_keys > 0 && 
               cur->next->keys[0] == key &&
               value > cur->values[cur->num_keys - 1]) {
            cur = cur->next;
        }

        insert_into_leaf(cur, key, value);

        if (cur->num_keys >= ORDER) {
            split_leaf(cur);
        }
    }

    void remove(const string& key, int value) {
        BPTreeNode* leaf = find_leaf(key);

        BPTreeNode* cur = leaf;
        while (cur) {
            for (int i = 0; i < cur->num_keys; i++) {
                if (cur->keys[i] == key && cur->values[i] == value) {
                    for (int j = i; j < cur->num_keys - 1; j++) {
                        cur->keys[j] = cur->keys[j+1];
                        cur->values[j] = cur->values[j+1];
                    }
                    cur->num_keys--;
                    return;
                }
                if (cur->keys[i] > key) return;
            }
            cur = cur->next;
        }
    }

    vector<int> find_all(const string& key) const {
        vector<int> result;

        if (root->is_leaf && root->num_keys == 0) return result;

        BPTreeNode* leaf = find_leaf(key);
        BPTreeNode* cur = leaf;

        while (cur) {
            for (int i = 0; i < cur->num_keys; i++) {
                if (cur->keys[i] == key) {
                    result.push_back(cur->values[i]);
                } else if (cur->keys[i] > key) {
                    return result;
                }
            }
            cur = cur->next;
        }

        return result;
    }

    bool load_from_file() {
        ifstream file(DATA_FILE, ios::binary);
        if (!file.is_open()) return false;

        int count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));

        for (int i = 0; i < count; i++) {
            int key_len;
            file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
            string key(key_len, '\0');
            file.read(&key[0], key_len);

            int value;
            file.read(reinterpret_cast<char*>(&value), sizeof(value));

            insert(key, value);
        }

        file.close();
        return true;
    }

    void save_to_file() {
        ofstream file(DATA_FILE, ios::binary | ios::trunc);

        vector<pair<string, int>> entries;
        collect_entries(root, entries);

        int count = entries.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& entry : entries) {
            int key_len = entry.first.size();
            file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            file.write(entry.first.c_str(), key_len);
            file.write(reinterpret_cast<const char*>(&entry.second), sizeof(entry.second));
        }

        file.close();
    }

private:
    void collect_entries(BPTreeNode* node, vector<pair<string, int>>& entries) const {
        if (!node) return;
        if (node->is_leaf) {
            for (int i = 0; i < node->num_keys; i++) {
                entries.push_back({node->keys[i], node->values[i]});
            }
        } else {
            for (int i = 0; i <= node->num_keys; i++) {
                collect_entries(node->children[i], entries);
            }
        }
    }
};

// ============================================================
// Main
// ============================================================

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    BPTree tree;
    tree.load_from_file();

    int n;
    cin >> n;

    string cmd, key;
    int value;

    for (int i = 0; i < n; i++) {
        cin >> cmd;

        if (cmd == "insert") {
            cin >> key >> value;
            tree.insert(key, value);
        } else if (cmd == "delete") {
            cin >> key >> value;
            tree.remove(key, value);
        } else if (cmd == "find") {
            cin >> key;
            vector<int> result = tree.find_all(key);

            if (result.empty()) {
                cout << "null\n";
            } else {
                for (size_t j = 0; j < result.size(); j++) {
                    if (j > 0) cout << ' ';
                    cout << result[j];
                }
                cout << '\n';
            }
        }
    }

    tree.save_to_file();

    return 0;
}
