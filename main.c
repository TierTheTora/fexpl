#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <ncurses.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>

#define PAD_T 4
#define PAD_L 20
#define T_STR 53

int cmp (const void* a, const void* b) {
        return strcmp(*(char**)a, *(char**)b);
}

int main (int argc, char ** argv) {
        setlocale(LC_ALL, "");
        initscr();
        refresh();
        noecho();
        cbreak();
        keypad(stdscr, 1);
        curs_set(0);
        start_color();
        use_default_colors();

        timeout(10);

        init_pair(1, COLOR_GREEN, -1);
        bkgd(COLOR_PAIR(1));

        int x, y;
        y = PAD_T;
        int pos = 0;
        int sndx = 0;
        char pwd[PATH_MAX];
        char dirname[PATH_MAX];
        snprintf(dirname, sizeof(dirname), "%s", getcwd(pwd, sizeof(pwd)));
	
        if (argc >= 2) {
                char *dirch = malloc((strlen(argv[1]) + 1) * sizeof(char));
                memset(dirch, 0, sizeof(dirch));
                strcpy(dirch, argv[1]);
                for (int i = 2; i < argc; ++i) {
                        dirch = realloc(dirch, strlen(dirch) + strlen(argv[i]) + 2);
                        strcat(dirch, " ");
                        strcat(dirch, argv[i]);
                }
                chdir(dirch);
                char cwd[PATH_MAX];
                snprintf(dirname, sizeof(dirname), "%s", getcwd(cwd, sizeof(cwd)));
                free(dirch);
        }
        
        while (1) {
                int dirc = 0;
                long long tsize = -8192;
                int c = getch();
                
                DIR *dir = opendir(dirname);
                struct dirent *de;
 
                char **dirsnf = NULL;
                while ((de = readdir(dir)) != NULL) {
                        dirsnf = realloc(dirsnf, (dirc + 1) * sizeof(char*));
                        dirsnf[dirc] = strdup(de->d_name);

                        ++dirc;
                }
                erase();
                qsort(dirsnf, dirc, sizeof(char*), cmp);
                for (int i = 0; i < dirc; ++i) {
                        if (i + sndx < dirc) {
                                char apnd = '\0';
                                char fullpath[PATH_MAX]; 
                                snprintf(fullpath, sizeof(fullpath) + 2, "%s/%s", dirname, dirsnf[i + sndx]);
                                DIR *dtmp = opendir(fullpath);
                                if (dtmp != NULL) { 
                                        apnd = '/';
                                        closedir(dtmp);
                                }
                                else if (access(fullpath, X_OK) == 0) {
                                        apnd = '*';
                                }
                                struct stat buf;
                                struct group  *gr = getgrgid(buf.st_gid);
                                struct tm *tm = localtime(&buf.st_mtime);
	                        
                                int res = stat(dirsnf[i + sndx], &buf);
 
                                char timebuf[64];
                                char prnmsg[128];
                                
                                strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm);
                                memset(prnmsg, 0, sizeof(prnmsg));
                                snprintf(prnmsg, sizeof(prnmsg), "%13d %s", (int)buf.st_size, timebuf);
                                tsize += buf.st_size;

                                x = strlen(prnmsg);
                                mvprintw(PAD_T + i + 1, 0, "%s", prnmsg);
                                mvprintw(PAD_T + i + 1, strlen(prnmsg), "  %s%c", dirsnf[i + sndx], apnd);
                        }
                }
                
                switch (c) {
                        case 'q':
                                if (dirsnf != NULL) {
                                        for (int i = 0; i < dirc; ++i) free(dirsnf[i]);
                                        free(dirsnf);
                                }

                                endwin();
                                exit(0);
                                break;
                        case KEY_UP: if (y > PAD_T) {
                                        --y;
                                        --pos;
                                        if (pos < 0) {
                                                pos = 0;
                                                sndx = 0;
                                        }
                                        if (sndx < 0) sndx = 0;
                                        if (sndx != 0) {
                                                --sndx;
                                                y = LINES - 2;
                                        }
                                }
                                if (y < PAD_T) y = PAD_T;
                                break;
                        case KEY_DOWN: if (pos < dirc - 1) {
                                        y++;
                                        pos++;
                                        if (y >= LINES - 1) {
                                                sndx++;
                                                y = LINES - 2;
                                        }
                                }
                                break;
                        case '\n': {
                                char path[PATH_MAX];
                                snprintf(path, sizeof(path) + 2, "%s/%s", dirname, dirsnf[pos]);
                                DIR *dtmp = opendir(path);
                                if (dtmp != NULL) {
                                        chdir(path);
                                        char cwd[PATH_MAX];
                                        snprintf(dirname, sizeof(dirname), "%s", getcwd(cwd, sizeof(cwd)));
                                        y = PAD_T;
                                        pos = 0;
                                        sndx = 0;
                                        closedir(dtmp);
                                }
                                else {
                                        char cmd[PATH_MAX];
                                        snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" > /dev/null 2>&1", dirsnf[pos]);

                                        if (fork() == 0) {
                                                system(cmd);
                                                exit(0);
                                        }
                                }
                                break;
                        }
                        case KEY_BACKSPACE:
                                chdir("..");
                                char cwd[PATH_MAX];
                                snprintf(dirname, sizeof(dirname), "%s", getcwd(cwd, sizeof(cwd)));
                                y = PAD_T;
                                pos = 0;
                                sndx = 0;
                                break;
                        case KEY_DC: {
                                int height = 8, width = 41;
                                int start_y = (LINES - height) / 2;
                                int start_x = (COLS - width) / 2;
                                WINDOW *win = newwin(height, width, start_y, start_x);
                                box(win, 0, 0);
                                mvwprintw(win, 3, 1, " Are you sure you want to delete this?");
                                mvwprintw(win, 4, 1, "            [Y] Yes  [N] No           ");
                                wrefresh(win);
                                int gch;
                                while (1) {
                                        gch = wgetch(win);
                                        if (gch == 'y') {
                                                struct stat sb;
                                                if (lstat(dirsnf[pos], &sb) == 0) {
                                                        if (S_ISDIR(sb.st_mode))
                                                                rmdir(dirsnf[pos]);
                                                        else
                                                                remove(dirsnf[pos]);
                                                }
                                                break;
                                        }
                                        else if (gch == 'n') {
                                                break;
                                        }
                                        else continue;
                                }
                                delwin(win);
                                y = PAD_T;
                                pos = 0;
                                sndx = 0;
                                continue;
                        }
                        case '+': {
                                char message[NAME_MAX];
                                echo();
                                timeout(-1);
                                
                                mvprintw(LINES - 2, 0, "File name: ");
                                move(LINES - 1, 0);
                                clrtoeol();
                                getnstr(message, sizeof(message) - 1);
                                
                                message[strlen(message)] = 0;
                                if (message[strlen(message) - 1] == '/') {
                                        message[strlen(message) - 1] = 0;
                                        mkdir(message, 0755);
                                }
                                else {
                                        int fd = open(message, O_WRONLY | O_CREAT, 0644);
                                        close(fd);
                                }

                                noecho();
                                timeout(10);
                                break;
                        }
                        case KEY_F(12): {
                                char message[NAME_MAX];
                                echo();
                                timeout(-1);
                                
                                mvprintw(LINES - 2, 0, "File name: ");
                                move(LINES - 1, 0);
                                clrtoeol();
                                getnstr(message, sizeof(message) - 1);
                                
                                message[strlen(message)] = 0;

                                rename(dirsnf[pos], message);

                                noecho();
                                timeout(10);
                        }
                }
                char name[PATH_MAX];
                closedir(dir);
                if (y < PAD_T) y = PAD_T;
                if (y >= dirc + PAD_T) y = dirc + 2;

                char apnd = '\0';
                char fullpath[PATH_MAX];
                snprintf(fullpath, sizeof(fullpath) + 2, "%s/%s", dirname, dirsnf[pos]);
                DIR *dtmp = opendir(fullpath);
                if (dtmp != NULL) { 
                        apnd = '/';
                        closedir(dtmp);
                }
                else if (access(fullpath, X_OK) == 0) {
                        apnd = '*';
                }
                snprintf(name, sizeof(name), " > %s%c", dirsnf[pos], apnd);
                mvprintw(y + 1, x, "%s", name);
                char cwd[PATH_MAX];
                mvprintw(2, PAD_L, "%s", dirname);
                mvprintw(3, PAD_L, "Total size: %lld", tsize);
                for (int i = 0; i < dirc; ++i)free(dirsnf[i]);  
                
                free(dirsnf);
                move(PAD_T, PAD_L);
                usleep(1000);
        }
        endwin();
        return 0;
}

