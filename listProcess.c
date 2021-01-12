#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <pwd.h>
#include <ctype.h>

#define PROC_ROOT "/proc"
void trim(char *str) 
{
    char *end = str;
    while(isspace((unsigned char)*end)) end++; 
    str = strcpy(str, end);

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';
}

struct tree_node {
    char name[128];
    char state[128];
    pid_t pid;
    pid_t ppid;
    int childCount; 
    struct tree_node* parent;
    struct tree_node* children[256];
    struct tree_node* next; // this is just to help us build the tree, use use *next as a list
};

static struct tree_node* p_head = NULL;

int parse_process(char *dirname) 
{
    struct tree_node *node = (struct tree_node*)malloc(sizeof(struct tree_node));
    char filename[256];
    char linebuf[256];
    char* key;
    char* value;
    strcpy(filename, dirname);
    strcat(filename, "/status"); //getting to the date of the process;
    FILE *p_file;
    p_file = fopen(filename, "r");
    if (p_file == NULL) {
        return 1; // prob just not a process folder
    }
    while (fgets(linebuf, sizeof(linebuf), p_file) != NULL) {
        key = strtok(linebuf, ":"); // using strtok and pointers
        value = strtok(NULL, ":");
        if (key != NULL && value != NULL) {
            trim(key);
            trim(value);
            if (strcmp(key, "Name") == 0) {
                strcpy (node->name, value);
            } else if (strcmp(key, "Pid") == 0) {
                node->pid =  atoi(value);
            } else if (strcmp(key, "PPid") == 0) {
                node->ppid =  atoi(value);
            } else if (strcmp(key, "State") == 0) {
                strcpy (node->state, value);
            }
        }
    }
    node->next = p_head;
    p_head = node; // so we have a way of getting to all processes
    return 0;
}

void build_tree(struct tree_node* source) 
{
    struct tree_node* father;
    struct tree_node* aux;
    struct tree_node* iter = source;
    while (iter != NULL) {
        // for each node in the list we find its father with aux
        //printf("Name: %s: %d: %d\n", iter->name, iter->pid, iter->ppid);
        aux = source;
        while (aux != NULL && aux->pid != iter->ppid) {
            aux = aux->next;
        }
        if (aux != NULL) {
            // This means aux is the father of iter
            iter->parent = aux;
            aux->children[aux->childCount] = iter;
            aux->childCount++;
        } else {
            // This means iter has no fathers -> he is the base process, the system
            father = iter;
        }
        iter = iter->next;
    }
    source = father;
}

void show_tree (struct tree_node *node, int padding, int index, int u)
{       
    //┬ ├ │└ ─
    int array[1024] = { [ 0 ... 1023 ] = 0 };
    int array1[1024] = { [ 0 ... 1023 ] = 0 };
    int ind=0;

    // First we print
    if (node -> ppid != -1) {
        if (padding == 0) {
            printf("%s─%d:%d", node->name, node->ppid, node->pid);
        } else {
            // finding out the distances and if we need │
            struct tree_node *current=node;
            while (current->parent != NULL)
            {
                current = current->parent;
                if (current->childCount > index)
                {
                    while (current->parent != NULL)
                    {
                        if (!(current == current->parent->children[current->parent->childCount - 1])) {
                            array1[ind]=1;
                        }
                        array[ind++] = strlen(current->name) + 2;
                        current = current->parent;
                    }
                }
                else
                {
                    array[ind++] = 0;
                }               
            }  
            // printing the spaces
            int sum=0;
            for (int j = ind - 1; j >= 0; j--)
                if (array1[j - 1] == 1 && j > 0) {
                    printf("%*c│", array[j], ' ');
                } else {
                    printf("%*c", array[j], ' ');
                }
            // here we print the processes
            if (node->childCount > 0)
            {
                if (u == 1) {
                    printf("└─%s⌄  %s %d:%d", node->name, node->state, node->ppid, node->pid);
                } else {
                    printf("├─%s⌄  %s %d:%d", node->name, node->state, node->ppid, node->pid);
                }
            } else {
                if (u == 1) {
                    printf("└─%s  %s %d:%d", node->name, node->state, node->ppid, node->pid);
                }
                else {
                    printf("├─%s  %s %d:%d", node->name, node->state, node->ppid, node->pid);
                }
            }
        }
        printf("\n");
    }
    // the dfs going through the nodes
    if(node -> ppid == -1) {
        for(int i = 0; i < node->childCount; ++i) {
            if (i == node->childCount - 1) {
                show_tree(node->children[i], 0, 0, 1);
            }
            else {
                show_tree(node->children[i], 0, 0, 0);
            }
        }
    } else {
        int cop=0;
        for(int i = 0; i < node->childCount; ++i) {
            int newPadding = padding;
            cop++;

            newPadding += strlen(node->name)+2;
            if (i == node->childCount - 1) {
                show_tree(node->children[i], newPadding, i, 1);
            } else {
                show_tree(node->children[i], newPadding, i, 0);
            }
        }
    }
}

int main(int argc, char **argv) 
{
    DIR* p_dir;
    struct dirent *dir_entry;
    char dirname[256];
    if ((p_dir = opendir(PROC_ROOT)) == NULL) {
        perror("Unable to open /proc");
        return 1;
    }
    struct tree_node *base = (struct tree_node*)malloc(sizeof(struct tree_node));
    strcpy(base->name, "Base");
    base->pid = 0;
    base->ppid = -1;
    p_head = base;
    // Reading the processes's files
    do {
        if ((dir_entry = readdir(p_dir)) != NULL) {
            if (dir_entry->d_type == DT_DIR) {
                strcpy(&dirname[0], PROC_ROOT);
                strcat(&dirname[0], "/");
                strcat(&dirname[0], dir_entry->d_name);
                parse_process(dirname);
            }
        }
    } while (dir_entry != NULL);

    // building the tree
    build_tree(p_head);
    p_head = base;

    // printing the tree
    show_tree(p_head, 0, 0, 0);
    return 0;
}


