#include "server.h"

const unsigned char IAC_IP[3] = "\xff\xf4";
const char* file_prefix = "./csie_trains/train_";
const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const char* welcome_banner = "======================================\n"
                             " Welcome to CSIE Train Booking System \n"
                             "======================================\n";

const char* lock_msg = ">>> Locked.\n";
const char* exit_msg = ">>> Client exit.\n";
const char* cancel_msg = ">>> You cancel the seat.\n";
const char* full_msg = ">>> The shift is fully booked.\n";
const char* seat_booked_msg = ">>> The seat is booked.\n";
const char* no_seat_msg = ">>> No seat to pay.\n";
const char* book_succ_msg = ">>> Your train booking is successful.\n";
const char* invalid_op_msg = ">>> Invalid operation.\n";

#ifdef READ_SERVER
char* read_shift_msg = "Please select the shift you want to check [902001-902005]: ";
#elif defined WRITE_SERVER
char* write_shift_msg = "Please select the shift you want to book [902001-902005]: ";
char* write_seat_msg = "Select the seat [1-40] or type \"pay\" to confirm: ";
char* write_seat_or_exit_msg = "Type \"seat\" to continue or \"exit\" to quit [seat/exit]: ";
#endif

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

int accept_conn(void);
// accept connection

static void getfilepath(char* filepath, int extension);
// get record filepath

double elapsed_time(struct timeval start) {
    struct timeval now;
    gettimeofday(&now, NULL);
    double elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1000000.0;
    return elapsed;
}

int check_write_lock(int fd, off_t start, off_t len) {
    struct flock lock;

    lock.l_type = F_WRLCK;   
    lock.l_whence = SEEK_SET; 
    lock.l_start = start;     
    lock.l_len = len;         

    fcntl(fd, F_GETLK, &lock);

    if (lock.l_type == F_UNLCK)
        return 0;
    else 
        return 1;
}

int unlock_region(int fd, off_t start, off_t len) {
    struct flock lock;

    lock.l_type = F_UNLCK;  
    lock.l_whence = SEEK_SET; 
    lock.l_start = start;     
    lock.l_len = len;   

    fcntl(fd, F_SETLK, &lock);
}


void lock_region(int fd, off_t start, off_t len) {
    struct flock lock;

    lock.l_type = F_WRLCK;   
    lock.l_whence = SEEK_SET; 
    lock.l_start = start;     
    lock.l_len = len;        

    fcntl(fd, F_SETLK, &lock);
}


int handle_read(request* reqP) {
    /*  Return value:
     *      1: read successfully
     *      0: read EOF (client down)
     *     -1: read failed
     *   TODO: handle incomplete input
     */
    int r;
    char buf[MAX_MSG_LEN];
    size_t len;

    memset(buf, 0, sizeof(buf));

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));

    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012"); // \r\n
    
    if (p1 == NULL) {
        p1 = strstr(buf, "\012");   // \n
        
        
        if (p1 == NULL) {
            if (!strncmp(buf, IAC_IP, 2)) {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client presses ctrl+C....\n");
                return 0;
            }

            if (reqP -> incomplete == 1)
                reqP -> buf_len = 0;

            memmove(reqP->buf + reqP->buf_len, buf, r);
            reqP -> buf_len += r;
            return 2;
        }
    }
    
    if (reqP -> incomplete == 1){
        len = p1 - buf + 1;
        memmove(reqP->buf, buf, len);
        reqP->buf[len - 1] = '\0';
        reqP->buf_len = len-1;
        return 1;
    }

    len = p1 - buf + 1;
    memmove(reqP->buf + reqP->buf_len, buf, len);
    reqP->buf[reqP->buf_len + len - 1] = '\0';
    reqP->buf_len += len - 1;
    return 1;
}

#ifdef READ_SERVER
int print_train_info(int conn_fd) {

    char buf[MAX_MSG_LEN];
    char read_buf[MAX_MSG_LEN];
    int temp[4] = {0};
    memset(buf, 0, sizeof(buf));
    memset(read_buf, 0, sizeof(read_buf));

    lseek(requestP[conn_fd].booking_info.train_fd, 0, SEEK_SET);
    read(requestP[conn_fd].booking_info.train_fd, read_buf, sizeof(read_buf));

    for (int i = 0; i < SEAT_NUM / 4; i++) {
        for (int j = 0; j < 4; j++){
            if (check_write_lock(requestP[conn_fd].booking_info.train_fd, i * 4 * 2 + j * 2, 1) == 1){
                temp[j] = 2;
            }
            else if (read_buf[i * 4 * 2 + j * 2] == '0')
                temp[j] = 0;
            else if (read_buf[i * 4 * 2 + j * 2] == '1')
                temp[j] = 1;
        }
        sprintf(buf + (i * 4 * 2), "%d %d %d %d\n", temp[0], temp[1], temp[2], temp[3]);
    }
    write(requestP[conn_fd].conn_fd, buf, strlen(buf));
    write(requestP[conn_fd].conn_fd, read_shift_msg, strlen(read_shift_msg));
    return 0;
}
#elif defined WRITE_SERVER
int print_train_info(int conn_fd, int seat, int lock_seat[5][40]) {
    /*
     * Booking info
     * |- Shift ID: 902001
     * |- Chose seat(s): 1,2
     * |- Paid: 3,4
     */
    int index = seat / 4 * 4 * 2 + seat % 4 * 2;
    char read_buf[MAX_MSG_LEN];
    bool full = true;
    memset(read_buf, 0, sizeof(read_buf));
    lseek(requestP[conn_fd].booking_info.train_fd, 0, SEEK_SET);
    read(requestP[conn_fd].booking_info.train_fd, read_buf, sizeof(read_buf));
    
    if (requestP[conn_fd].status == SHIFT){
        for (int i = 0; i < SEAT_NUM / 4; i++) {
            for (int j = 0; j < 4; j++){
                if (read_buf[i * 4 * 2 + j * 2] == '0'){
                    full = false;
                    break;
                }
            }
            if (!full)
                break;
        }
        if (full){
            write(requestP[conn_fd].conn_fd, full_msg, strlen(full_msg));
            write(requestP[conn_fd].conn_fd, write_shift_msg, strlen(write_shift_msg));
            return SHIFT;
        }
    }

    if (requestP[conn_fd].status == SEAT){
        if (requestP[conn_fd].booking_info.seat_stat[seat] == CHOSEN){
            write(requestP[conn_fd].conn_fd, cancel_msg, strlen(cancel_msg));
            requestP[conn_fd].booking_info.seat_stat[seat] = UNKNOWN;
            lock_seat[requestP[conn_fd].booking_info.shift_id - 902001][seat] = 0;
            unlock_region(requestP[conn_fd].booking_info.train_fd, index, 1);
        }
        else if (read_buf[index] == '1'){
            write(requestP[conn_fd].conn_fd, seat_booked_msg, strlen(seat_booked_msg));
        }
        else if (check_write_lock(requestP[conn_fd].booking_info.train_fd, index, 1) == 1 || lock_seat[requestP[conn_fd].booking_info.shift_id - 902001][seat] == 1){
            write(requestP[conn_fd].conn_fd, lock_msg, strlen(lock_msg));
        }
        else {
            lock_region(requestP[conn_fd].booking_info.train_fd, index, 1);
            lock_seat[requestP[conn_fd].booking_info.shift_id - 902001][seat] = 1;
            requestP[conn_fd].booking_info.seat_stat[seat] = CHOSEN;
        }
    }

    char buf[MAX_MSG_LEN*3];
    char chosen[MAX_MSG_LEN] = "";
    char paid[MAX_MSG_LEN] = "";
    char temp[5] = {0};

    for (int i = 0; i < 40; i++){
        if (requestP[conn_fd].booking_info.seat_stat[i] == CHOSEN){
            sprintf(temp, "%d", i + 1);
            if (chosen[0] == '\0'){
                strcat(chosen, temp);
            }
            else {
                strcat(chosen, ",");
                strcat(chosen, temp);
            }
        }
        if (requestP[conn_fd].booking_info.seat_stat[i] == PAID){
            sprintf(temp, "%d", i + 1);
            if (paid[0] == '\0'){
                strcat(paid, temp);
            }
            else {
                strcat(paid, ",");
                strcat(paid, temp);
            }
        }
    }

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "\nBooking info\n"
                 "|- Shift ID: %d\n"
                 "|- Chose seat(s): %s\n"
                 "|- Paid: %s\n\n"
                 ,requestP[conn_fd].booking_info.shift_id, chosen, paid);
    write(requestP[conn_fd].conn_fd, buf, strlen(buf));
    if (requestP[conn_fd].status != BOOKED)
        write(requestP[conn_fd].conn_fd, write_seat_msg, strlen(write_seat_msg));
    return SEAT;
}
#endif

void close_conn(int conn_fd, int lock_seat[5][40]){
#ifdef WRITE_SERVER
    for (int i = 0; i < 40; i++){
        if (requestP[conn_fd].booking_info.seat_stat[i] == CHOSEN){
            unlock_region(requestP[conn_fd].booking_info.train_fd, i / 4 * 4 * 2 + i % 4 * 2, 1);
            lock_seat[requestP[conn_fd].booking_info.shift_id - 902001][i] = 0;
            requestP[conn_fd].booking_info.seat_stat[i] = UNKNOWN;
        }
    }
#endif
    requestP[conn_fd].buf_len = 0;
    requestP[conn_fd].booking_info.train_fd = -1;
    requestP[conn_fd].booking_info.shift_id = 902001;
    requestP[conn_fd].status = SHIFT;
    close(requestP[conn_fd].conn_fd);
    free_request(&requestP[conn_fd]);
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    int conn_fd;  // fd for file that we open for reading
    char buf[MAX_MSG_LEN*2], filename[FILE_LEN];
    
    int now_i = 0;
    int lock_seat[5][40];
    for (int i = 0; i < 5; i++){
        for (int j = 0; j < 40; j++){
            lock_seat[i][j] = 0;
        }
    }
    int i,j;

    for (i = TRAIN_ID_START, j = 0; i <= TRAIN_ID_END; i++, j++) {
        getfilepath(filename, i);
#ifdef READ_SERVER
        trains[j].file_fd = open(filename, O_RDONLY);
#elif defined WRITE_SERVER
        trains[j].file_fd = open(filename, O_RDWR);
#else
        trains[j].file_fd = -1;
#endif
        if (trains[j].file_fd < 0) {
            ERR_EXIT("open");
        }
    }

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    struct pollfd *fds = (struct pollfd *) malloc((maxfd) * sizeof(struct pollfd));
    for (int i = 0; i < maxfd; i++) {
        fds[i].fd = -1;  
        fds[i].events = 0;
        fds[i].revents = 0;
    }

    qnode *head = (qnode *) malloc (sizeof(qnode));
    qe *time_queue = (qe *) malloc (sizeof(qe));
    time_queue -> head = head;
    time_queue -> rear = head;
    head -> next = NULL;
    head -> prev = NULL;

    fds[0].fd = svr.listen_fd; 
    fds[0].events = POLLIN;
    alive_conn = 1;

    

    while (1) {
        // TODO: Add IO multiplexing
        int timeout;
        if (time_queue -> head -> next == NULL)
            timeout = -1;
        else
            timeout = (5 - elapsed_time(time_queue -> head -> next -> start_time)) * 1000 + 1;
        poll(fds, alive_conn, timeout);

        qnode *free_node = NULL;
        for (qnode *temp = time_queue -> head -> next; temp != NULL;){
            if (elapsed_time(temp -> start_time) > 5){
                close_conn(temp -> conn_fd, lock_seat);
                time_queue -> head -> next = temp -> next;
                if (temp -> next != NULL)
                    temp -> next -> prev = time_queue -> head;
                fds[temp -> fd_index].fd = -1;
                free_node = temp;
                if (temp == time_queue -> rear)
                    time_queue -> rear = time_queue -> head;
                temp = temp -> next;
                free(free_node);
            }
            else 
                break;
        }

        conn_fd = -1;

        if (fds[0].revents & POLLIN) {
            conn_fd = accept_conn();  
            if (conn_fd < 0) {
                continue;
            }
            requestP[conn_fd].status = SHIFT;
            requestP[conn_fd].booking_info.train_fd = -1;
            requestP[conn_fd].booking_info.shift_id = 902001;
            for (int i = 0; i < 40; i++)
                requestP[conn_fd].booking_info.seat_stat[i] = UNKNOWN;
            

            for (int i = 1; i <= maxfd; i++) {
                if (fds[i].fd == -1) {
                    fds[i].fd = conn_fd;
                    fds[i].events = POLLIN; 
                    qnode *new = (qnode *) malloc (sizeof (qnode));
                    time_queue -> rear -> next = new;
                    new -> prev = time_queue -> rear;
                    new -> conn_fd = conn_fd;
                    new -> fd_index = i;
                    new -> next = NULL;
                    time_queue -> rear = new;
                    requestP[conn_fd].time_node = new;
                    requestP[conn_fd].incomplete = 1;
                    gettimeofday(&(new -> start_time), NULL);
                    if (i >= alive_conn)
                        alive_conn = i + 1;
                    break;
                }
            }
            write(requestP[conn_fd].conn_fd, welcome_banner, strlen(welcome_banner));
#ifdef READ_SERVER      
            write(requestP[conn_fd].conn_fd, read_shift_msg, strlen(read_shift_msg));
#elif defined WRITE_SERVER
            write(requestP[conn_fd].conn_fd, write_shift_msg, strlen(write_shift_msg));    
#endif           
            continue;
        }

        

        for (int i = 1; i < alive_conn; i++) {
            if (fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
               conn_fd = fds[i].fd;
               now_i = i;
               break;
            }
        }
            

        if (conn_fd == -1)
            continue;


        int ret = handle_read(&requestP[conn_fd]);
        requestP[conn_fd].incomplete = ret;
        if (ret < 0) {
            fprintf(stderr, "bad request from %s\n", requestP[conn_fd].host);
            continue;
        }
        if (ret == 0){
            if (now_i == alive_conn - 1)
                alive_conn -= 1;
            fds[now_i].fd = -1;
            requestP[conn_fd].time_node -> prev -> next = requestP[conn_fd].time_node -> next;
            
            if (requestP[conn_fd].time_node -> next != NULL)
                requestP[conn_fd].time_node -> next -> prev = requestP[conn_fd].time_node -> prev;
            else 
                time_queue -> rear = requestP[conn_fd].time_node -> prev;
            free(requestP[conn_fd].time_node);
            close_conn(conn_fd, lock_seat);
            continue;
        }
        if (ret == 2)
            continue;

        // TODO: handle requests from clients
#ifdef READ_SERVER      
        if (strcmp(requestP[conn_fd].buf, "902001") == 0){
            requestP[conn_fd].booking_info.train_fd = trains[0].file_fd;
            print_train_info(conn_fd);
        }
        else if (strcmp(requestP[conn_fd].buf, "902002") == 0){
            requestP[conn_fd].booking_info.train_fd = trains[1].file_fd;
            print_train_info(conn_fd);
        }
        else if (strcmp(requestP[conn_fd].buf, "902003") == 0){
            requestP[conn_fd].booking_info.train_fd = trains[2].file_fd;
            print_train_info(conn_fd);
        }
        else if (strcmp(requestP[conn_fd].buf, "902004") == 0){
            requestP[conn_fd].booking_info.train_fd = trains[3].file_fd;
            print_train_info(conn_fd);
        }
        else if (strcmp(requestP[conn_fd].buf, "902005") == 0){
            requestP[conn_fd].booking_info.train_fd = trains[4].file_fd;
            print_train_info(conn_fd);
        }
        else if (strcmp(requestP[conn_fd].buf, "exit") == 0){
            write(requestP[conn_fd].conn_fd, exit_msg, strlen(exit_msg));
            if (now_i == alive_conn - 1)
                alive_conn -= 1;
            fds[now_i].fd = -1;
            requestP[conn_fd].time_node -> prev -> next = requestP[conn_fd].time_node -> next;
            
            if (requestP[conn_fd].time_node -> next != NULL)
                requestP[conn_fd].time_node -> next -> prev = requestP[conn_fd].time_node -> prev;
            else 
                time_queue -> rear = requestP[conn_fd].time_node -> prev;
            free(requestP[conn_fd].time_node);
            close_conn(conn_fd, lock_seat);
            continue;
        }
        else {
            write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
            if (now_i == alive_conn - 1)
                alive_conn -= 1;
            fds[now_i].fd = -1;
            requestP[conn_fd].time_node -> prev -> next = requestP[conn_fd].time_node -> next;
            
            if (requestP[conn_fd].time_node -> next != NULL)
                requestP[conn_fd].time_node -> next -> prev = requestP[conn_fd].time_node -> prev;
            else 
                time_queue -> rear = requestP[conn_fd].time_node -> prev;
            free(requestP[conn_fd].time_node);
            close_conn(conn_fd, lock_seat);
            continue;
        }

        /*
        sprintf(buf,"%s : %s",accept_read_header,requestP[conn_fd].buf);
        write(requestP[conn_fd].conn_fd, buf, strlen(buf));
        */
#elif defined WRITE_SERVER
        
        if (requestP[conn_fd].status == SHIFT){
            if (strcmp(requestP[conn_fd].buf, "902001") == 0){
                requestP[conn_fd].booking_info.train_fd = trains[0].file_fd;
                requestP[conn_fd].booking_info.shift_id = 902001;
                requestP[conn_fd].status = print_train_info(conn_fd, 0, lock_seat);
            }
            else if (strcmp(requestP[conn_fd].buf, "902002") == 0){
                requestP[conn_fd].booking_info.train_fd = trains[1].file_fd;
                requestP[conn_fd].booking_info.shift_id = 902002;
                requestP[conn_fd].status = print_train_info(conn_fd, 0, lock_seat);
            }
            else if (strcmp(requestP[conn_fd].buf, "902003") == 0){
                requestP[conn_fd].booking_info.train_fd = trains[2].file_fd;
                requestP[conn_fd].booking_info.shift_id = 902003;

                requestP[conn_fd].status = print_train_info(conn_fd, 0, lock_seat);
            }
            else if (strcmp(requestP[conn_fd].buf, "902004") == 0){
                requestP[conn_fd].booking_info.train_fd = trains[3].file_fd;
                requestP[conn_fd].booking_info.shift_id = 902004;
                requestP[conn_fd].status = print_train_info(conn_fd, 0, lock_seat);
            }
            else if (strcmp(requestP[conn_fd].buf, "902005") == 0){
                requestP[conn_fd].booking_info.train_fd = trains[4].file_fd;
                requestP[conn_fd].booking_info.shift_id = 902005;
                requestP[conn_fd].status = print_train_info(conn_fd, 0, lock_seat);
            }
            else if (strcmp(requestP[conn_fd].buf, "exit") == 0){
                if (now_i == alive_conn - 1)
                    alive_conn -= 1;
                fds[now_i].fd = -1;
                requestP[conn_fd].time_node -> prev -> next = requestP[conn_fd].time_node -> next;
                
                if (requestP[conn_fd].time_node -> next != NULL)
                    requestP[conn_fd].time_node -> next -> prev = requestP[conn_fd].time_node -> prev;
                else 
                    time_queue -> rear = requestP[conn_fd].time_node -> prev;
                free(requestP[conn_fd].time_node);
                write(requestP[conn_fd].conn_fd, exit_msg, strlen(exit_msg));
                close_conn(conn_fd, lock_seat);
                continue;
            }
            else {
                write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                if (now_i == alive_conn - 1)
                    alive_conn -= 1;
                fds[now_i].fd = -1;
                requestP[conn_fd].time_node -> prev -> next = requestP[conn_fd].time_node -> next;
                
                if (requestP[conn_fd].time_node -> next != NULL)
                    requestP[conn_fd].time_node -> next -> prev = requestP[conn_fd].time_node -> prev;
                else 
                    time_queue -> rear = requestP[conn_fd].time_node -> prev;
                free(requestP[conn_fd].time_node);
                
                close_conn(conn_fd, lock_seat);
                continue;
            }
            continue;
        }
        char temp[5];
        memset(temp, 0, sizeof(temp));
        int seat = 0;
        bool pay = false;
        if (requestP[conn_fd].status == SEAT){
            if (strcmp(requestP[conn_fd].buf, "pay") == 0){
                for (int i = 0; i < 40; i++){
                    if (requestP[conn_fd].booking_info.seat_stat[i] == CHOSEN){
                        requestP[conn_fd].booking_info.seat_stat[i] = PAID;
                        lock_seat[requestP[conn_fd].booking_info.shift_id - 902001][i] = 0;
                        
                        lseek(requestP[conn_fd].booking_info.train_fd, i / 4 * 4 * 2 + i % 4 * 2, SEEK_SET);
                        write(requestP[conn_fd].booking_info.train_fd, "1", 1);
                        unlock_region(requestP[conn_fd].booking_info.train_fd, i / 4 * 4 * 2 + i % 4 * 2, 1);
                        pay = true;
                    }
                }
                if (pay){
                    requestP[conn_fd].status = BOOKED;
                    write(requestP[conn_fd].conn_fd, book_succ_msg, strlen(book_succ_msg));
                    print_train_info(conn_fd, 0, lock_seat);
                    write(requestP[conn_fd].conn_fd, write_seat_or_exit_msg, strlen(write_seat_or_exit_msg));
                    continue;
                }
                else {
                    write(requestP[conn_fd].conn_fd, no_seat_msg, strlen(no_seat_msg));
                    requestP[conn_fd].status = BOOKED;
                    print_train_info(conn_fd, 0, lock_seat);
                    requestP[conn_fd].status = SEAT;
                    write(requestP[conn_fd].conn_fd, write_seat_msg, strlen(write_seat_msg));
                    continue;
                }
            }
            if (strcmp(requestP[conn_fd].buf, "exit") == 0){
                write(requestP[conn_fd].conn_fd, exit_msg, strlen(exit_msg));
                if (now_i == alive_conn - 1)
                    alive_conn -= 1;
                fds[now_i].fd = -1;
                requestP[conn_fd].time_node -> prev -> next = requestP[conn_fd].time_node -> next;
                
                if (requestP[conn_fd].time_node -> next != NULL)
                    requestP[conn_fd].time_node -> next -> prev = requestP[conn_fd].time_node -> prev;
                else 
                    time_queue -> rear = requestP[conn_fd].time_node -> prev;
                free(requestP[conn_fd].time_node);
                close_conn(conn_fd, lock_seat);
                continue;
            }
            for (int i = 1; i <= 40; i++){
                sprintf(temp, "%d", i);
                if (strcmp(requestP[conn_fd].buf, temp) == 0){
                    seat = i;
                    break;
                }
            }
            if (seat == 0){
                write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                if (now_i == alive_conn - 1)
                    alive_conn -= 1;
                fds[now_i].fd = -1;
                requestP[conn_fd].time_node -> prev -> next = requestP[conn_fd].time_node -> next;
                
                if (requestP[conn_fd].time_node -> next != NULL)
                    requestP[conn_fd].time_node -> next -> prev = requestP[conn_fd].time_node -> prev;
                else 
                    time_queue -> rear = requestP[conn_fd].time_node -> prev;
                free(requestP[conn_fd].time_node);
                close_conn(conn_fd, lock_seat);
                continue;
            }
            print_train_info(conn_fd, seat - 1, lock_seat);
        }

        if (requestP[conn_fd].status == BOOKED){
            if (strcmp(requestP[conn_fd].buf, "seat") == 0){
                requestP[conn_fd].status = BOOKED;
                print_train_info(conn_fd, 0, lock_seat);
                requestP[conn_fd].status = SEAT;
                write(requestP[conn_fd].conn_fd, write_seat_msg, strlen(write_seat_msg));
            }
            else if (strcmp(requestP[conn_fd].buf, "exit") == 0){
                write(requestP[conn_fd].conn_fd, exit_msg, strlen(exit_msg));
                if (now_i == alive_conn - 1)
                    alive_conn -= 1;
                fds[now_i].fd = -1;
                requestP[conn_fd].time_node -> prev -> next = requestP[conn_fd].time_node -> next;
                
                if (requestP[conn_fd].time_node -> next != NULL)
                    requestP[conn_fd].time_node -> next -> prev = requestP[conn_fd].time_node -> prev;
                else 
                    time_queue -> rear = requestP[conn_fd].time_node -> prev;
                free(requestP[conn_fd].time_node);
                close_conn(conn_fd, lock_seat);
                continue;
            }
            else {
                write(requestP[conn_fd].conn_fd, invalid_op_msg, strlen(invalid_op_msg));
                if (now_i == alive_conn - 1)
                    alive_conn -= 1;
                fds[now_i].fd = -1;
                requestP[conn_fd].time_node -> prev -> next = requestP[conn_fd].time_node -> next;
                
                if (requestP[conn_fd].time_node -> next != NULL)
                    requestP[conn_fd].time_node -> next -> prev = requestP[conn_fd].time_node -> prev;
                else 
                    time_queue -> rear = requestP[conn_fd].time_node -> prev;
                free(requestP[conn_fd].time_node);
                close_conn(conn_fd, lock_seat);
                continue;
            }
        }
        /*
        sprintf(buf,"%s : %s",accept_write_header,requestP[conn_fd].buf);
        write(requestP[conn_fd].conn_fd, buf, strlen(buf));    
        */
#endif
    }      

    free(requestP);
    close(svr.listen_fd);
    for (i = 0;i < TRAIN_NUM; i++)
        close(trains[i].file_fd);

    return 0;
}

int accept_conn(void) {

    struct sockaddr_in cliaddr;
    size_t clilen;
    int conn_fd;  // fd for a new connection with client

    clilen = sizeof(cliaddr);
    conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
    if (conn_fd < 0) {
        if (errno == EINTR || errno == EAGAIN) return -1;  // try again
        if (errno == ENFILE) {
            (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                return -1;
        }
        ERR_EXIT("accept");
    }
    
    requestP[conn_fd].conn_fd = conn_fd;
    strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
    fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
    requestP[conn_fd].client_id = (svr.port * 1000) + num_conn;    // This should be unique for the same machine.
    num_conn++;
    
    return conn_fd;
}

static void getfilepath(char* filepath, int extension) {
    char fp[FILE_LEN*2];
    
    memset(filepath, 0, FILE_LEN);
    sprintf(fp, "%s%d", file_prefix, extension);
    strcpy(filepath, fp);
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->client_id = -1;
    reqP->buf_len = 0;
    reqP->status = INVALID;
    reqP->remaining_time.tv_sec = 5;
    reqP->remaining_time.tv_usec = 0;

    reqP->booking_info.num_of_chosen_seats = 0;
    reqP->booking_info.train_fd = -1;
    for (int i = 0; i < SEAT_NUM; i++)
        reqP->booking_info.seat_stat[i] = UNKNOWN;
}

static void free_request(request* reqP) {
    memset(reqP, 0, sizeof(request));
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}