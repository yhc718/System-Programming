#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "hw2.h"

#define ERR_EXIT(s) perror(s), exit(errno);

/*
If you need help from TAs,
please remember :
0. Show your efforts
    0.1 Fully understand course materials
    0.2 Read the spec thoroughly, including frequently updated FAQ section
    0.3 Use online resources
    0.4 Ask your friends while avoiding plagiarism, they might be able to understand you better, since the TAs know the solution, 
        they might not understand what you're trying to do as quickly as someone who is also doing this homework.
1. be respectful
2. the quality of your question will directly impact the value of the response you get.
3. think about what you want from your question, what is the response you expect to get
4. what do you want the TA to help you with. 
    4.0 Unrelated to Homework (wsl, workstation, systems configuration)
    4.1 Debug
    4.2 Logic evaluation (we may answer doable yes or no, but not always correct or incorrect, as it might be giving out the solution)
    4.3 Spec details inquiry
    4.4 Testcase possibility
5. If the solution to answering your question requires the TA to look at your code, you probably shouldn't ask it.
6. We CANNOT tell you the answer, but we can tell you how your current effort may approach it.
7. If you come with nothing, we cannot help you with anything.
*/

// somethings I recommend leaving here, but you may delete as you please
static char root[MAX_FRIEND_INFO_LEN] = "Not_Tako";     // root of tree
static char friend_info[MAX_FRIEND_INFO_LEN];   // current process info
static char friend_name[MAX_FRIEND_NAME_LEN];   // current process name
static int friend_value;    // current process value
FILE* read_fp = NULL;
FILE* write_fp = NULL;
int parent_fd[2];
int end_nums = 0;
bool end[MAX_CHILDREN] = {0};
int childs = 0;
FILE *child_stream[MAX_CHILDREN][2];

// Is Root of tree
static inline bool is_Not_Tako() {
    return (strcmp(friend_name, root) == 0);
}

// a bunch of prints for you
void print_direct_meet(char *friend_name) {
    fprintf(stdout, "Not_Tako has met %s by himself\n", friend_name);
}

void print_indirect_meet(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "Not_Tako has met %s through %s\n", child_friend_name, parent_friend_name);
}

void print_fail_meet(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "Not_Tako does not know %s to meet %s\n", parent_friend_name, child_friend_name);
}

void print_fail_check(char *parent_friend_name){
    fprintf(stdout, "Not_Tako has checked, he doesn't know %s\n", parent_friend_name);
}

void print_success_adopt(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "%s has adopted %s\n", parent_friend_name, child_friend_name);
}

void print_fail_adopt(char *parent_friend_name, char *child_friend_name) {
    fprintf(stdout, "%s is a descendant of %s\n", parent_friend_name, child_friend_name);
}

void print_compare_gtr(char *friend_name){
    fprintf(stdout, "Not_Tako is still friends with %s\n", friend_name);
}

void print_compare_leq(char *friend_name){
    fprintf(stdout, "%s is dead to Not_Tako\n", friend_name);
}

void print_final_graduate(){
    fprintf(stdout, "Congratulations! You've finished Not_Tako's annoying tasks!\n");
}

/* terminate child pseudo code
void clean_child(){
    close(child read_fd);
    close(child write_fd);
    call wait() or waitpid() to reap child; // this is blocking
}

*/

/* remember read and write may not be fully transmitted in HW1?
void fully_write(int write_fd, void *write_buf, int write_len);

void fully_read(int read_fd, void *read_buf, int read_len);

please do above 2 functions to save some time
*/

int main(int argc, char *argv[]) {
    // Hi! Welcome to SP Homework 2, I hope you have fun
    pid_t process_pid = getpid(); // you might need this when using fork()
    
    if (argc != 2) {
        fprintf(stderr, "Usage: ./friend [friend_info]\n");
        return 0;
    }
    
    setvbuf(stdout, NULL, _IONBF, 0); // prevent buffered I/O, equivalent to fflush() after each stdout, study this as you may need to do it for other friends against their parents
    
    // put argument one into friend_info
    strncpy(friend_info, argv[1], MAX_FRIEND_INFO_LEN);
    
    if(strcmp(argv[1], root) == 0){
        // is Not_Tako
        strncpy(friend_name, friend_info,MAX_FRIEND_NAME_LEN);      // put name into friend_nae
        friend_name[MAX_FRIEND_NAME_LEN - 1] = '\0';        // in case strcmp messes with you
        read_fp = stdin;        // takes commands from stdin
        friend_value = 100;     // Not_Tako adopting nodes will not mod their values
    }
    else{
        /*
        parent_fd[0] = atoi(getenv("PIPE_READ"));
        parent_fd[1] = atoi(getenv("PIPE_WRITE"));
        */
        parent_fd[0] = 3;
        parent_fd[1] = 4;
        char *split_point = strchr(friend_info, '_');
        strncpy(friend_name, friend_info, split_point - friend_info);
        friend_name[split_point - friend_info] = '\0';
        friend_value = atoi(split_point + 1);
        read_fp = fdopen(parent_fd[0], "r");
        write_fp = fdopen(parent_fd[1], "w");
        setvbuf(write_fp, NULL, _IONBF, 0);
        fputs("okay\n", write_fp);
        // is other friends
        // extract name and value from info
        // where do you read from?
        // anything else you have to do before you start taking commands?
    }

    //TODO:
    /* you may follow SOP if you wish, but it is not guaranteed to consider every possible outcome

    1. read from parent/stdin
    2. determine what the command is (Meet, Check, Adopt, Graduate, Compare(bonus)), I recommend using strcmp() and/or char check
    3. find out who should execute the command (extract information received)
    4. execute the command or tell the requested friend to execute the command
        4.1 command passing may be required here
    5. after previous command is done, repeat step 1.
    */

    // Hint: do not return before receiving the command "Graduate"
    // please keep in mind that every process runs this exact same program, so think of all the possible cases and implement them
    
    
    while (true){
        char input[MAX_CMD_LEN] = {0};
        fgets(input, sizeof(input), read_fp);
        if (strlen(input) == 0){
            continue;
        }
        if (strcmp(input, "Exit\n") == 0){
            for (int i = 0; i < childs; i++){
                fputs("Exit\n", child_stream[i][1]);
                wait(NULL);
            }
            exit(0);
        }

        char *space = strchr(input, ' ');
        if (space == NULL){
            fprintf(stderr, "%s received error input : %s\n", friend_name, input);
            continue;
        }
        char command[MAX_CMD_LEN] = {0};



        strncpy(command, input, space - input);
        command[space - input] = '\0';

        if (strcmp(command, "FIFO") == 0){
            char num[10] = {0};
            char *end_string = strchr(space + 1, '\n');
            if (end_string == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(num, space + 1, end_string - space - 1);
            num[end_string - space - 1] = '\0';
            int layers = atoi(num);
            char fifo_info[30] = {0};
            sprintf(fifo_info, "%s %d\n", friend_info, layers);
            FILE *fifo_fp = fopen("Adopt.fifo", "w");
            setvbuf(fifo_fp, NULL, _IONBF, 0);
            fputs(fifo_info, fifo_fp);
            fclose(fifo_fp);
            char new_command[30] = {0};
            sprintf(new_command, "FIFO %d\n", layers + 1);
            for (int i = 0; i < childs; i++){
                fputs(new_command, child_stream[i][1]);
                char feedback[10] = {0};
                fgets(feedback, sizeof(feedback), child_stream[i][0]);
            }
            fputs("okay\n", write_fp);
        }
        else if (strcmp(command, "Name") == 0){
            char name[MAX_FRIEND_NAME_LEN] = {0};
            char *end_string = strchr(space + 1, '\n');
            if (end_string == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(name, space + 1, end_string - space - 1);
            name[end_string - space - 1] = '\0';
            if (strcmp(name, friend_name) == 0)
                fputs("Yes\n", write_fp);
            else {
                for (int i = 0; i < childs; i++)
                    fputs(input, child_stream[i][1]);

                char feedback[10] = {0};
                bool fail = false;
                for (int i = 0; i < childs; i++){
                    fgets(feedback, sizeof(feedback), child_stream[i][0]);
                    if (strcmp(feedback, "Yes\n") == 0)
                        fail = true;
                }

                if (fail)
                    fputs("Yes\n", write_fp);
                else 
                    fputs("No\n", write_fp);
            }
        }
        else if (strcmp(command, "write") == 0){
            char num[10] = {0};
            char *end_string = strchr(space + 1, '\n');
            if (end_string == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(num, space + 1, end_string - space - 1);
            num[end_string - space - 1] = '\0';
            int layers = atoi(num);
            if (layers == 0){
                fprintf(stdout, "%s", friend_info);
                if (childs == 0)  
                    fputs("ends\n", write_fp);
                else
                    fputs("okay\n", write_fp);
            }
            else {
                char write_buffer[10] = {0};
                sprintf(write_buffer, "write %d\n", layers - 1);
                bool print = false;
                for (int i = 0; i < childs; i++){
                    if (end[i] == true)
                        continue;
                    if (print == true)
                        printf(" ");
                    print = true;
                    char feedback[10] = {0};
                    fputs(write_buffer, child_stream[i][1]);
                    fgets(feedback, sizeof(feedback), child_stream[i][0]);
                    if (strcmp(feedback, "ends\n") == 0){
                        end[i] = true;
                        end_nums ++;
                    }
                }
                if (end_nums == childs) {
                    fputs("ends\n", write_fp);
                    end_nums = 0;
                    for (int i = 0; i < 8; i++)
                        end[i] = false;
                }
                else
                    fputs("okay\n", write_fp);

            }
        }
        else if (strcmp(command, "Meet") == 0){
            char name[MAX_FRIEND_NAME_LEN] = {0};
            char *second_space = strchr(space + 1, ' ');
            if (second_space == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(name, space + 1, second_space - space - 1);
            name[second_space - space - 1] = '\0';
            char new_info[MAX_FRIEND_INFO_LEN] = {0};
            char *end_string = strchr(second_space + 1, '\r');
            if (end_string == NULL)
                end_string = strchr(second_space + 1, '\n');
            if (end_string == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(new_info, second_space + 1, end_string - second_space - 1);
            new_info[end_string - second_space - 1] = '\0';

            char *split_point = strchr(new_info, '_');
            if (split_point == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            char new_name[MAX_FRIEND_NAME_LEN] = {0};
            strncpy(new_name, new_info, split_point - new_info);
            new_name[split_point - new_info] = '\0';

            if (strcmp(name, friend_name) == 0){
                // create new child node
                if (strcmp(friend_name, "Not_Tako") == 0)
                    print_direct_meet(new_name);
                else 
                    print_indirect_meet(friend_name, new_name);
                int pipes[2][2];
                pipe(pipes[0]);
                pipe(pipes[1]);
                int pid;
                if ((pid = fork()) < 0)
                    printf("error\n");
                else if (pid == 0){
                    close(pipes[0][1]);
                    close(pipes[1][0]);
                    if (strcmp(friend_name, "Not_Tako") != 0){
                        fclose(read_fp);
                        fclose(write_fp);
                    }
                    for (int i = 0; i < childs; i++){
                        fclose(child_stream[i][0]);
                        fclose(child_stream[i][1]);
                    }

                    if (pipes[0][0] != 3){
                        dup2(pipes[0][0], 3);
                        close(pipes[0][0]);
                    }

                    if (pipes[1][1] != 4){
                        dup2(pipes[1][1], 4);
                        close(pipes[1][1]);
                    }
                    /*
                    char pipe_read_fd[10] = {0};
                    char pipe_write_fd[10] = {0};
                    sprintf(pipe_read_fd, "%d", pipes[0][0]);
                    sprintf(pipe_write_fd, "%d", pipes[1][1]);
                    setenv("PIPE_READ", pipe_read_fd, 1);
                    setenv("PIPE_WRITE", pipe_write_fd, 1);
                    */
                    execl("./friend", "friend", new_info, (char *) NULL);
                }
                else {
                    // close read or write fd for pipe?
                    close(pipes[0][0]);
                    close(pipes[1][1]);
                    FILE *stream = fdopen(pipes[1][0], "r");
                    child_stream[childs][0] = stream;
                    stream = fdopen(pipes[0][1], "w");
                    setvbuf(stream, NULL, _IONBF, 0);
                    child_stream[childs][1] = stream;
                    char feedback[10] = {0};
                    fgets(feedback, sizeof(feedback), child_stream[childs][0]);
                    if (strcmp(friend_name, "Not_Tako") != 0)
                        fputs("Success\n", write_fp);
                    
                    childs ++;
                }
            }

            else {
                // fprintf(stdout, "%s\n", friend_info);
                for (int i = 0; i < childs; i++)
                    fputs(input, child_stream[i][1]);

                bool success = false;
                char feedback[10] = {0};
                for (int i = 0; i < childs; i++){
                   
                    fgets(feedback, sizeof(feedback), child_stream[i][0]);
                    if (strcmp("Success\n", feedback) == 0)
                        success = true;
    
                }
                if (strcmp("Not_Tako", friend_name) == 0){
                    if (!success)
                        print_fail_meet(name, new_name);
                }
                else {
                    if (success)
                        fputs("Success\n", write_fp);
                    else {
                        fputs("Fail\n", write_fp);
                    }
                }
                    
                // traverse all child nodes
            }
        }
        else if (strcmp(command, "meet") == 0){
            char name[MAX_FRIEND_NAME_LEN] = {0};
            char *second_space = strchr(space + 1, ' ');
            if (second_space == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(name, space + 1, second_space - space - 1);
            name[second_space - space - 1] = '\0';
            char new_info[MAX_FRIEND_INFO_LEN] = {0};
            char *end_string = strchr(second_space + 1, '\r');
            if (end_string == NULL)
                end_string = strchr(second_space + 1, '\n');
            if (end_string == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(new_info, second_space + 1, end_string - second_space - 1);
            new_info[end_string - second_space - 1] = '\0';

            char *split_point = strchr(new_info, '_');
            if (split_point == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            char new_name[MAX_FRIEND_NAME_LEN] = {0};
            strncpy(new_name, new_info, split_point - new_info);
            new_name[split_point - new_info] = '\0';

            if (strcmp(name, friend_name) == 0){
                // create new child node
                int pipes[2][2];
                pipe(pipes[0]);
                pipe(pipes[1]);
                int pid;
                if ((pid = fork()) < 0)
                    printf("error\n");
                else if (pid == 0){
                    close(pipes[0][1]);
                    close(pipes[1][0]);
                    if (strcmp(friend_name, "Not_Tako") != 0){
                        fclose(read_fp);
                        fclose(write_fp);
                    }
                    for (int i = 0; i < childs; i++){
                        fclose(child_stream[i][0]);
                        fclose(child_stream[i][1]);
                    }

                    if (pipes[0][0] != 3){
                        dup2(pipes[0][0], 3);
                        close(pipes[0][0]);
                    }

                    if (pipes[1][1] != 4){
                        dup2(pipes[1][1], 4);
                        close(pipes[1][1]);
                    }
                    /*
                    char pipe_read_fd[10] = {0};
                    char pipe_write_fd[10] = {0};
                    sprintf(pipe_read_fd, "%d", pipes[0][0]);
                    sprintf(pipe_write_fd, "%d", pipes[1][1]);
                    setenv("PIPE_READ", pipe_read_fd, 1);
                    setenv("PIPE_WRITE", pipe_write_fd, 1);
                    */
                    execl("./friend", "friend", new_info, (char *) NULL);
                }
                else {
                    // close read or write fd for pipe?
                    close(pipes[0][0]);
                    close(pipes[1][1]);
                    FILE *stream = fdopen(pipes[1][0], "r");
                    child_stream[childs][0] = stream;
                    stream = fdopen(pipes[0][1], "w");
                    setvbuf(stream, NULL, _IONBF, 0);
                    child_stream[childs][1] = stream;
                    char feedback[10] = {0};
                    fgets(feedback, sizeof(feedback), child_stream[childs][0]);
                    childs ++;
                    if (strcmp(friend_name, "Not_Tako") != 0)
                        fputs("Success\n", write_fp);
                    
                    
                }
            }

            else {
                // fprintf(stdout, "%s\n", friend_info);
                for (int i = 0; i < childs; i++)
                    fputs(input, child_stream[i][1]);

                bool success = false;
                char feedback[10] = {0};
                for (int i = 0; i < childs; i++){
                   
                    fgets(feedback, sizeof(feedback), child_stream[i][0]);
                    if (strcmp("Success\n", feedback) == 0)
                        success = true;
    
                }
                if (strcmp("Not_Tako", friend_name) == 0){
                    if (!success)
                        print_fail_meet(name, new_name);
                }
                else {
                    if (success)
                        fputs("Success\n", write_fp);
                    else {
                        fputs("Fail\n", write_fp);
                    }
                }
                    
                // traverse all child nodes
            }
        }
        else if (strcmp(command, "Check") == 0){
            char name[MAX_FRIEND_NAME_LEN] = {0};
            char *end_string = strchr(space + 1, '\r');
            if (end_string == NULL)
                end_string = strchr(space + 1, '\n');
            if (end_string == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(name, space + 1, end_string - space - 1);
            name[end_string - space - 1] = '\0';

            if (strcmp(name, friend_name) == 0){
                fprintf(stdout, "%s\n", friend_info);
                for (int i = 0; end_nums < childs; i++){
                    bool print = false;
                    for (int j = 0; j < childs; j++){
                        if (end[j] == true)
                            continue;
                        if (print == true)
                            printf(" ");
                        print = true;
                        char feedback[10] = {0};
                        char write_buffer[10] = {0};
                        sprintf(write_buffer, "write %d\n", i);
                        fputs(write_buffer, child_stream[j][1]);
                        fgets(feedback, sizeof(feedback), child_stream[j][0]);
                        if (strcmp(feedback, "ends\n") == 0){
                            end[j] = true;
                            end_nums ++;
                        }
                    }
                    printf("\n");
                }
                end_nums = 0;
                for (int i = 0; i < 8; i++){
                    end[i] = false;
                }
                if (strcmp(friend_name, "Not_Tako") != 0)
                    fputs("Success\n", write_fp);
            }

            else {
                for (int i = 0; i < childs; i++)
                    fputs(input, child_stream[i][1]);

                bool success = false;
                char feedback[10] = {0};
                for (int i = 0; i < childs; i++){
                   
                    fgets(feedback, sizeof(feedback), child_stream[i][0]);
                    if (strcmp("Success\n", feedback) == 0)
                        success = true;
    
                }
                if (strcmp("Not_Tako", friend_name) == 0){
                    if (!success)
                        print_fail_check(name);
                }
                else {
                    if (success)
                        fputs("Success\n", write_fp);
                    else {
                        fputs("Fail\n", write_fp);
                    }
                }


            }
        }
        else if (strcmp(command, "Graduate") == 0){
            char name[MAX_FRIEND_NAME_LEN] = {0};
            char *end_string = strchr(space + 1, '\r');
            if (end_string == NULL)
                end_string = strchr(space + 1, '\n');
            if (end_string == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(name, space + 1, end_string - space - 1);
            name[end_string - space - 1] = '\0';

            char new_command[MAX_CMD_LEN] = {0};
            
            sprintf(new_command, "Check %s\n", name);

            if (strcmp("Not_Tako", friend_name) == 0 && strcmp("Not_Tako", name) != 0){

                for (int i = 0; i < childs; i++)
                    fputs(new_command, child_stream[i][1]);
                for (int i = 0; i < childs; i++){
                    char feedback[10] = {0};
                    fgets(feedback, sizeof(feedback), child_stream[i][0]);
                }

            }

            else if (strcmp("Not_Tako", friend_name) == 0){
                fprintf(stdout, "%s\n", friend_info);
                for (int i = 0; end_nums < childs; i++){
                    bool print = false;
                    for (int j = 0; j < childs; j++){
                        if (end[j] == true)
                            continue;
                        if (print == true)
                            printf(" ");
                        print = true;
                        char feedback[10] = {0};
                        char write_buffer[10] = {0};
                        sprintf(write_buffer, "write %d\n", i);
                        fputs(write_buffer, child_stream[j][1]);
                        fgets(feedback, sizeof(feedback), child_stream[j][0]);
                        if (strcmp(feedback, "ends\n") == 0){
                            end[j] = true;
                            end_nums ++;
                        }
                    }
                    printf("\n");
                }
                end_nums = 0;
                for (int i = 0; i < 8; i++){
                    end[i] = false;
                }
            }


            if (strcmp(name, friend_name) == 0){
                for (int i = 0; i < childs; i++){
                    fputs("Exit\n", child_stream[i][1]);
                    wait(NULL);
                }
                if (strcmp(name, "Not_Tako") == 0){
                    print_final_graduate();
                    exit(0);
                }
                else {
                    fputs("End\n", write_fp);
                    exit(0);
                }
            }

            else {
                for (int i = 0; i < childs; i++)
                    fputs(input, child_stream[i][1]);

                bool success = false;
                int index = -1;
                char feedback[10] = {0};
                for (int i = 0; i < childs; i++){
                   
                    fgets(feedback, sizeof(feedback), child_stream[i][0]);
                    if (strcmp("Success\n", feedback) == 0)
                        success = true;
                    if (strcmp("End\n", feedback) == 0){
                        wait(NULL);
                        success = true;
                        index = i;
                    }
    
                }

                if (strcmp("Not_Tako", friend_name) == 0){
                    if (!success)
                        print_fail_check(name);
                }
                else {
                    if (success)
                        fputs("Success\n", write_fp);
                    else {
                        fputs("Fail\n", write_fp);
                    }
                }
                

                if (index != -1){
                    childs --;
                    fclose(child_stream[index][0]);
                    fclose(child_stream[index][1]);
                    for (int i = index; i < childs; i++){
                        child_stream[i][0] = child_stream[i + 1][0];
                        child_stream[i][1] = child_stream[i + 1][1];
                    }
                }
            }


        }
        else if (strcmp(command, "Adopt") == 0){
            if (strcmp(friend_name, "Not_Tako") == 0)
                mkfifo("Adopt.fifo", 0666);
            char parent_name[MAX_FRIEND_NAME_LEN] = {0};
            char *second_space = strchr(space + 1, ' ');
            if (second_space == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(parent_name, space + 1, second_space - space - 1);
            parent_name[second_space - space - 1] = '\0';
            char child_name[MAX_FRIEND_INFO_LEN] = {0};
            char *end_string = strchr(second_space + 1, '\r');
            if (end_string == NULL)
                end_string = strchr(second_space + 1, '\n');
            if (end_string == NULL){
                fprintf(stderr, "%s received error input : %s\n", friend_name, input);
                continue;
            }
            strncpy(child_name, second_space + 1, end_string - second_space - 1);
            child_name[end_string - second_space - 1] = '\0';

            if (strcmp(child_name, friend_name) == 0){
                char write_buffer[30] = {0};
                sprintf(write_buffer, "Name %s\n", parent_name);
                for (int i = 0; i < childs; i++)
                    fputs(write_buffer, child_stream[i][1]);

                bool fail = false;
                char feedback[10] = {0};
                for (int i = 0; i < childs; i++){
            
                    fgets(feedback, sizeof(feedback), child_stream[i][0]);
                    if (strcmp("Yes\n", feedback) == 0)
                        fail = true;
                    
                }
                if (fail){
                    print_fail_adopt(parent_name, child_name);
                    unlink("Adopt.fifo");
                }
                else {

                    char fifo_info[30] = {0};
                    char feedback[10] = {0};
                    sprintf(fifo_info, "%s 1\n", friend_info);
                    FILE *fifo_fp = fopen("Adopt.fifo", "w");
                    setvbuf(fifo_fp, NULL, _IONBF, 0);
                    fputs(fifo_info, fifo_fp);
                    fclose(fifo_fp);
                    

                    for (int i = 0; i < childs; i++){
                        fputs("FIFO 2\n", child_stream[i][1]);
                        fgets(feedback, sizeof(feedback), child_stream[i][0]);
                    }
                    fifo_fp = fopen("Adopt.fifo", "w");
                    setvbuf(fifo_fp, NULL, _IONBF, 0);
                    fputs("End\n", fifo_fp);
                    fclose(fifo_fp);
                }

                if (strcmp(friend_name, "Not_Tako") != 0){
                    if (!fail){
                        char name_buffer[30] = {0};
                        sprintf(name_buffer, "%s\n", friend_name);
                        fputs(name_buffer, write_fp);
                    }
                    else 
                        fputs("okay\n", write_fp);
                }
                // write FIFO
            }
            else {
                for (int i = 0; i < childs; i++)
                    fputs(input, child_stream[i][1]);
            }

            if (strcmp(parent_name, friend_name) == 0){
                
                
                
                char parent_table[8][MAX_FRIEND_NAME_LEN] = {0};
                char temp_info[MAX_FRIEND_INFO_LEN] = {0};
                char temp_name[MAX_FRIEND_NAME_LEN] = {0};
                strcpy(parent_table[0], friend_name);
                FILE *fifo_fp = fopen("Adopt.fifo", "r");
                while (true){
                    char read_buffer[30] = {0};
                    fgets(read_buffer, sizeof(read_buffer), fifo_fp);
                    
                    if (strlen(read_buffer) == 0){
                        clearerr(fifo_fp);
                        usleep(1000);
                        continue;
                    }
                    
                    // fprintf(stdout, "%s", read_buffer);
                    if (strcmp("End\n", read_buffer) == 0){
                        fclose(fifo_fp);
                        unlink("Adopt.fifo");
                        break;
                    }
                    char *fifo_space = strchr(read_buffer, ' ');
                    strncpy(temp_info, read_buffer, fifo_space - read_buffer);
                    temp_info[fifo_space - read_buffer] = '\0';
                    char *temp_info_split = strchr(temp_info, '_');
                    strncpy(temp_name, temp_info, temp_info_split - temp_info);
                    temp_name[temp_info_split - temp_info] = '\0';
                    int new_value = atoi(temp_info_split + 1) % friend_value;
                    int layer = atoi(fifo_space + 1);
                    char new_info[MAX_FRIEND_INFO_LEN] = {0};
                    if (new_value < 10)
                        sprintf(new_info, "%s_0%d", temp_name, new_value);
                    else 
                        sprintf(new_info, "%s_%d", temp_name, new_value);
                    strncpy(parent_table[layer], temp_name, strlen(temp_name));
                    parent_table[layer][strlen(temp_name)] = '\0';
                    if (layer == 1){
                        int pipes[2][2];
                        pipe(pipes[0]);
                        pipe(pipes[1]);
                        int pid;
                        if ((pid = fork()) == 0){
                            close(pipes[0][1]);
                            close(pipes[1][0]);
                            if (strcmp(friend_name, "Not_Tako") != 0){
                                fclose(read_fp);
                                fclose(write_fp);
                            }
                            for (int i = 0; i < childs; i++){
                                fclose(child_stream[i][0]);
                                fclose(child_stream[i][1]);
                            }

                            if (pipes[0][0] != 3){
                                dup2(pipes[0][0], 3);
                                close(pipes[0][0]);
                            }

                            if (pipes[1][1] != 4){
                                dup2(pipes[1][1], 4);
                                close(pipes[1][1]);
                            }
                            /*
                            char pipe_read_fd[10] = {0};
                            char pipe_write_fd[10] = {0};
                            sprintf(pipe_read_fd, "%d", pipes[0][0]);
                            sprintf(pipe_write_fd, "%d", pipes[1][1]);
                            setenv("PIPE_READ", pipe_read_fd, 1);
                            setenv("PIPE_WRITE", pipe_write_fd, 1);
                            */
                            execl("./friend", "friend", new_info, (char *) NULL);
                        }
                        else {
                            // close read or write fd for pipe?
                            close(pipes[0][0]);
                            close(pipes[1][1]);
                            FILE *stream = fdopen(pipes[1][0], "r");
                            child_stream[childs][0] = stream;
                            stream = fdopen(pipes[0][1], "w");
                            setvbuf(stream, NULL, _IONBF, 0);
                            child_stream[childs][1] = stream;
                            char feedback[10] = {0};
                            fgets(feedback, sizeof(feedback), child_stream[childs][0]);
                            childs ++;
                        }
                    }
                    else {
                        char meet_buffer[MAX_CMD_LEN] = {0};
                        sprintf(meet_buffer, "meet %s %s\n", parent_table[layer - 1], new_info);
                        fputs(meet_buffer, child_stream[childs - 1][1]);
                        char feedback[10] = {0};
                        fgets(feedback, sizeof(feedback), child_stream[childs - 1][0]);
                    }
                }
                print_success_adopt(parent_name, child_name);
                // read FIFO and do Meet until "End\n"
            }

            if (strcmp(friend_name, child_name) != 0){
                int index = -1;
                int adjustment = 0;
                if (strcmp(friend_name, parent_name) == 0)
                    adjustment = 1;
                
                for (int i = 0; i < childs - adjustment; i++){
                    char feedback[10] = {0};
                    fgets(feedback, sizeof(feedback), child_stream[i][0]);
                    if (strcmp(feedback, "okay\n") != 0){
                        char write_buffer[30] = {0};
                        sprintf(write_buffer, "Graduate %s", feedback);
                        fputs(write_buffer, child_stream[i][1]);
                        fgets(feedback, sizeof(feedback), child_stream[i][0]);
                        wait(NULL);
                        fclose(child_stream[i][0]);
                        fclose(child_stream[i][1]);
                        index = i;
                    }
                }
                
                if (index != -1){
                    childs --;
                    for (int i = index; i < childs; i++){
                        child_stream[i][0] = child_stream[i + 1][0];
                        child_stream[i][1] = child_stream[i + 1][1];
                    }
                }
                

                if (strcmp(friend_name, "Not_Tako") != 0)
                    fputs("okay\n", write_fp);
            }

            
        }
        /* pseudo code
        if(Meet){
            create array[2]
            make pipe
            use fork.
                Hint: remember to fully understand how fork works, what it copies or doesn't
            check if you are parent or child
            as parent or child, think about what you do next.
                Hint: child needs to run this program again
        }
        else if(Check){
            obtain the info of this subtree, what are their info?
            distribute the info into levels 1 to 7 (refer to Additional Specifications: subtree level <= 7)
            use above distribution to print out level by level
                Q: why do above? can you make each process print itself?
                Hint: we can only print line by line, is DFS or BFS better in this case?
        }
        else if(Graduate){
            perform Check
            terminate the entire subtree
                Q1: what resources have to be cleaned up and why?
                Hint: Check pseudo code above
                Q2: what commands needs to be executed? what are their orders to avoid deadlock or infinite blocking?
                A: (tell child to die, reap child, tell parent you're dead, return (die))
        }
        else if(Adopt){
            remember to make fifo
            obtain the info of child node subtree, what are their info?
                Q: look at the info you got, how do you know where they are in the subtree?
                Hint: Think about how to recreate the subtree to design your info format
            A. terminate the entire child node subtree
            B. send the info through FIFO to parent node
                Q: why FIFO? will usin pipe here work? why of why not?
                Hint: Think about time efficiency, and message length
            C. parent node recreate the child node subtree with the obtained info
                Q: which of A, B and C should be done first? does parent child position in the tree matter?
                Hint: when does blocking occur when using FIFO?(mkfifo, open, read, write, unlink)
            please remember to mod the values of the subtree, you may use bruteforce methods to do this part (I did)
            also remember to print the output
        }
        else if(full_cmd[1] == 'o'){
            Bonus has no hints :D
        }
        else{
            there's an error, we only have valid commmands in the test cases
            fprintf(stderr, "%s received error input : %s\n", friend_name, full_cmd); // use this to print out what you received
        }
        */
    }

   // final print, please leave this in, it may bepart of the test case output
    if(is_Not_Tako()){
        print_final_graduate();
    }
    return 0;
}