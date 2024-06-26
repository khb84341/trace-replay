#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <iostream>
#include <string>
#include "simpleReplay.h"
#include <cstring>

using namespace std;

#define MAX_NAME	200

int mkdir_all_path(const char *path);
int create_init_files(FILE* init_fp);

double trace_replay(char *input_name);
int file_create(const char *path);
int file_mkdir(char *path);
int file_unlink(char *path);
int file_rmdir(char *path);
int file_fsync(char *path, int option);
int file_rename(char *path1, char *path2);
int file_write(const char *path, long long int write_off, 
			long long int write_size, long long int file_size);
int file_append(const char *path, long long int write_off, 
				long long int write_size, long long int file_size);
int create_camera_file(char* camera_path, unsigned long long size);
int delete_camera_file(char* camera_path);
int file_truncate(const char *path,
				long long int after_size, long long int before_size);
int file_truncate_same(const char *path,
				long long int after_size, long long int before_size);
int file_read(char *path, long long int read_off, long long int read_size, char* name);

double IG_lasttime;
int IG_trace(string strLine);

static int create_cache_file(struct ReplayJob *replay, string path);
static int test_and_create_cache(struct ReplayJob *replay, string trace_path, long long int file_size);
static int test_cache(struct ReplayJob *replay, string trace_path);
static int search_cache_file(struct ReplayJob *replay, string trace_path, string *new_path);
static void remove_dir_caches(struct ReplayJob *replay, string trace_path);
static int update_cache_file(struct ReplayJob *replay, string trace_path, string new_path);
static int search_db_type(struct ReplayJob *replay, string trace_path);
static int insert_recent_cache(struct CacheDirInfo *cacheDirInfo, string trace_path);

int print_starttrace_fsync(double curTime);
int print_time_fsync(clock_t start_time, clock_t end_time);
int print_time_read(char* name, clock_t start_time, clock_t end_time); // update

static int database_append(struct ReplayJob *replay, string trace_path, long long int write_off, long long int write_size, long long int file_size);
static int database_overwrite(struct ReplayJob *replay, string trace_path, long long int write_off, long long int write_size, long long int file_size);
static int database_truncate(struct ReplayJob *replay, string trace_path, long long int after_size, long long int before_size);
static int database_unlink(struct ReplayJob *replay, string trace_path);



//char prefix[PATH_MAX + 1];

#define DEBUG_REPLAY	1

struct trace_stat
{
	int create;
	int mkdir;
	int unlink;
	int rmdir;
	int fsync;
	int rename;
	int write_overwrite;
	int write_append;
};

/*
void print_help()
{
	printf("simpleReplay 1.1 Version\n");
	printf("Usage: ./simpleReplay [OPTION] [TRACENAME]\n");
	printf("[OPTION]\n");
	printf(" -i [getFileList output]: initialize files\n");
	printf(" -C [dir]: Create Multimedia Dummy File to dir\n");
	printf(" -s [size(byte)]: Multimedia Size (default:2MB)\n");
	printf(" -R [dir]: Remove 1 multimedia file(random) in dir\n");
	printf(" -p [path]: prefix path (mount dir)\n");
	printf(" -v: print log\n");
}

int main (int argc, char *argv[])
{
	FILE* init_fp;
	char init_name[MAX_NAME];
	char camera_path[PATH_MAX];
	int opt, i;
	unsigned long long camera_size = 4194304; 

	mode_flag = 0;
	memset(prefix, 0, PATH_MAX + 1);

	while ((opt = getopt(argc, argv, "hi:C:R:p:s:v")) != EOF) {
		switch (opt) {
		case 'h':
			print_help();
			goto out;
		case 'i': 
			strncpy(init_name, optarg, MAX_NAME);
			init_fp= fopen(init_name, "r");
			if (init_fp == NULL) {
				printf("Error: Can not open init file: %s\n", init_name);
				goto out;
			}
			mode_flag |= INIT_FILE;
			break;
		case 'C':
			strncpy(camera_path, optarg, PATH_MAX);
			mode_flag |= TAKE_PICTURE;
			break;
		case 's':
			camera_size = atoll(optarg);
			break;
		case 'R':
			strncpy(camera_path, optarg, PATH_MAX);
			mode_flag |= DELETE_PICTURE;
			break;
		case 'p':
			strncpy(prefix, optarg, PATH_MAX);
			mode_flag |= PREFIX;
			break;
		case 'v':
			mode_flag |= VERBOSE; 
			break;
		default:
			print_help();
			goto out;
		}
	}

	if (mode_flag & INIT_FILE)
		create_init_files(init_fp);
	if (mode_flag & TAKE_PICTURE)
		create_camera_file(camera_path, camera_size);
	if (mode_flag & DELETE_PICTURE)
		delete_camera_file(camera_path);

	for (i = optind; i < argc; i++)
	{
//		printf("Replay %s\n", argv[i]);
		trace_replay(argv[i]);	
	}

out:

	return 0;
}
*/

int create_camera_file(char* camera_path, unsigned long long size)
{
	return 0;
}

int create_camera_file(char *mount_dir, char* camera_path, char* ext_path, unsigned long long size)
{
	char path[PATH_MAX];
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);

	if (IG_mode == 1) {
		return 0;
	}

	memset(path, 0, PATH_MAX);
	sprintf(path, "%s/%s/mul_%ld_%ld.%s", mount_dir, camera_path, cur_time.tv_sec, cur_time.tv_usec, ext_path);

	file_create(path);
	file_write(path, 0, size, 0);

	return 0;
}

int delete_camera_file(char* camera_path)
{
	return 0;
}

int delete_camera_file(char* mount_dir, char* camera_path)
{
	char path[PATH_MAX];
	char camera_path_full[PATH_MAX];
	DIR *dir_info;
	struct dirent *dir_entry;
	int count = 0;
	int value;

	if (IG_mode == 1) {
		return 0;
	}

	memset(camera_path_full, 0, PATH_MAX);
	sprintf(camera_path_full, "%s/%s", mount_dir, camera_path);

	dir_info = opendir(camera_path_full);
	if (dir_info != NULL)
	{
		while (dir_entry = readdir(dir_info))
		{
			if (dir_entry->d_type != DT_REG)
				continue;
			if (strstr(dir_entry->d_name, "mul_") == NULL)
				continue;
			count++;
		}
		closedir(dir_info);
	}

	if (count == 0)
		return 0;

	value = rand() % 10;
	if (value < 8) {
		if (count / 4 > 0)
			value = count - (rand() % (count/4)) - 1;
		else
			value = rand() % count;
		if (value < 0)
			value = 0;
	}
	else 	
		value = rand() % count;

	count = 0;

	dir_info = opendir(camera_path_full);
	if (dir_info != NULL)
	{
		while (dir_entry = readdir(dir_info))
		{
			if (dir_entry->d_type != DT_REG)
				continue;
			if (strstr(dir_entry->d_name, "mul_") == NULL)
				continue;
			count++;
			if (count == value) {
				sprintf(path, "%s/%s/%s", mount_dir, camera_path, dir_entry->d_name);
				file_unlink(path);
				break;
			}
		}
	closedir(dir_info);
	}
}

int create_init_files(FILE* init_fp)
{
	return 0;
}

int create_init_files(char* mount_dir, char* init_name)
{
	char line[PATH_MAX];
	int line_count = 0;
	int new_fd;
	int ret;
	FILE* init_fp = fopen(init_name, "r");
	if (init_fp == NULL) {
		printf("Error: Can not open init file: %s\n", init_name);
		return -1;
	}

	if (IG_mode)
		return 0;

	while (fgets(line, PATH_MAX, init_fp) != NULL)
	{
		const char *tmp;
		char path[PATH_MAX];
		char path_input[PATH_MAX];
		char path_target[PATH_MAX];
		char path_target_input[PATH_MAX];
		char *ptr;
		unsigned long long inode_num;
		unsigned long long size;
#define TYPE_REG	0
#define TYPE_LINK	1
		int file_type;

		line_count = line_count + 1;

		// cout << line_count << " " << line;

		if (line[2] != '/' || line[3] != 'd' ||
				line[4] != 'a' || line[5] != 't' || line[6] != 'a')
			continue;

		if (line[0] == 'R')
			file_type = TYPE_REG;
		else if (line[0] == 'L')
			file_type = TYPE_LINK;
		else
			continue;

		ptr = strtok(line, "\t");
		if (ptr == NULL)
			continue;
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			continue;
		strncpy(path, ptr, strlen(ptr));
		path[strlen(ptr)] = 0x00;

		if (file_type == TYPE_REG)
		{
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			inode_num = atoll(ptr);

			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			size = atoll(ptr);
		}
		else if (file_type == TYPE_LINK)
		{
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			strncpy(path_target, ptr, strlen(ptr));
			path_target[strlen(ptr)] = 0x00;
		}

		tmp = path;
		tmp += 1;
//		tmp += 6;

		memset(path_input, 0, PATH_MAX);
		sprintf(path_input, "%s/%s", mount_dir, tmp);

		if (file_type == TYPE_REG)
		{
			new_fd = open(path_input, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
			if (new_fd < 0)
			{
				mkdir_all_path(path_input);
				new_fd = open(path_input, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
				if (new_fd < 0) {
					printf("Failed: Create %s %d\n", path_input, errno);
					continue;
				}
			}

			ret = fallocate64(new_fd, 0, 0, size);
			if (ret < 0) {
				continue;
			}
			close(new_fd);

			long long int write_off = 0;
			long long int write_size = 4096;
			while (write_off < size) {
				if (write_off + write_size < size)
					write_size = size - write_off;
				file_write(path_input, write_off, write_size, size);
				write_off += write_size;
			}
		}
		else if (file_type == TYPE_LINK)
		{
			if (path_target[0] != '/')
				continue;

			tmp = path_target;
			tmp += 1;

			memset(path_target_input, 0, PATH_MAX);
			sprintf(path_target_input, "%s/%s", mount_dir, tmp);
			mkdir_all_path(path_target_input);

			symlink(path_input, path_target_input);
		}
	}
	return 0;
}

int mkdir_all_path(const char *path)
{
	const char *tmp = path;
	char tmp_path[PATH_MAX];
	int len = 0;
	int ret;

	if (path == NULL)
		return -1;
	
	while ((tmp = strchr(tmp, '/')) != NULL) {
		len = tmp - path;
		tmp++;
		if (len == 0)
			continue;
		memset(tmp_path, 0, PATH_MAX);
		strncpy(tmp_path, path, len);
		tmp_path[len] = 0x00;
		
		if ((ret = mkdir(tmp_path, 0776)) == -1) {
			if (errno != EEXIST) {
				return -1;
			}
		}
	}
	return 0;
}

double trace_replay(char *input_name)
{
	return 0;
}

static double __do_trace_replay(string strMount, char* line, struct ReplayJob *replay, double curTime=0.0, int isUpdate = 0);
static double __do_trace_replay(string strMount, char* line, struct ReplayJob *replay, double curTime, int isUpdate)
{
	double last_time;
	bool isCache = false;
	char* ptr;
	char* ptr2;
	char type[10];
	char tmp[PATH_MAX + 1];
	char path[PATH_MAX + 1];
	const char* mount_dir = strMount.c_str();
	string strLine = string(line);
	char *name = "asdf";

	if (replay != NULL) {
		if (replay->type == REPLAY_LOADING)
			isCache = true;
		else if ((replay->type == REPLAY_UPDATE))
			isCache = true;
		else if ((replay->type == REPLAY_BG))
			isCache = true;
	}
	// cout << line;

	ptr = strtok(line, "\t");
	if (ptr == NULL)
		return -1;
	last_time = strtod(ptr, &ptr2);

	if (IG_mode == 1) {
		double itime = last_time - IG_lasttime;
		if (itime > 300)
			itime = 300;
		else if (itime < 0)
			itime = 1;
		IG_curTime += itime;
		IG_trace(strLine);
		IG_lasttime = last_time;
		return 0;
	}

	ptr = strtok(NULL, "\t");
	if (ptr == NULL)
		return -1;
	strncpy(type, ptr, strlen(ptr));
	type[strlen(ptr)] = 0x00;

	ptr = strtok(NULL, "\t");
	if (ptr == NULL)
		return -1;
	strncpy(tmp, ptr, strlen(ptr));
	tmp[strlen(ptr)] = 0x00;

	memset(path, 0, PATH_MAX);
	sprintf(path, "%s/data%s", mount_dir, tmp);

	if (strncmp(type, "[R]", 4) != 0) {
		if (isUpdate == 0) {
			string str_path = string(path);
			if (str_path.find("/app/") != std::string::npos){
				return -1;
			}
		}
	}

	if (strncmp(type, "[CR]", 4) == 0) {
/*
		string str_path = string(path);
		if (isCache) {
			if (create_cache_file(replay, str_path) < 0)
				file_create(path);
		}
		else
			file_create(path);
*/
	}
	else if (strncmp(type, "[MD]", 4) == 0) {
		file_mkdir(path);
	}
	else if (strncmp(type, "[UN]", 4) == 0) {
		int isUnlink = 1; 
		if (isCache) {
			string new_path;	
			string str_path = string(path);
			if (search_cache_file(replay, str_path, &new_path) == 0) {
				isUnlink = 0;
				// cout << "[UN]:" << str_path << ":not delete" << endl;
			}
			database_unlink(replay, string(path));
		}
		if (isUnlink == 1) {
			file_unlink(path);
		}
	}
	else if (strncmp(type, "[RD]", 4) == 0) {
		string str_path = string(path);
		if (isCache) {
			if (test_cache(replay, str_path) == 0) {}
			else {
				file_rmdir(path);
				remove_dir_caches(replay, str_path);
			}
		}
		else
			file_rmdir(path);
	}
	else if (strncmp(type, "[FS]", 4) == 0)
	{
		int sync_option;
		clock_t start_time, end_time;
		string str_path = string(path);

		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		sync_option = atoi(ptr);
		fflush(stdout);
		
		if (isCache) {
			string new_path;
			if (search_cache_file(replay, str_path, &new_path) == 0) {
				memset(path, 0, PATH_MAX);
				sprintf(path, "%s", new_path.c_str());
				// cout << "[FS]" << str_path << "->" << new_path << endl;
			}
		}
		start_time = clock();
		file_fsync(path, sync_option);
		end_time = clock();
		if (mode_flag & FSYNCTIME)
			print_time_fsync(start_time, end_time);
	}
	else if (strncmp(type, "[RN]", 4) == 0)
	{
		char path2[PATH_MAX + 1];
		struct stat stat_buf;
		int isSrcCache = 0, isDstCache = 0;
		string src_path, dst_path;
		string src_cache, dst_cache;

		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		strncpy(tmp, ptr, strlen(ptr));
		tmp[strlen(ptr)] = 0x00;
		memset(path2, 0, PATH_MAX);
		sprintf(path2, "%s/data/%s", mount_dir, tmp);

		src_path = string(path);
		dst_path = string(path2);

		if ((stat(path, &stat_buf) >= 0) && isCache) 
		{
			if (S_ISDIR(stat_buf.st_mode)) {} 
			else {
				if (search_cache_file(replay, src_path, &src_cache) == 0) {
					memset(path, 0, PATH_MAX);
					sprintf(path, "%s", src_cache.c_str());
					isSrcCache = 1;
				}
				if (search_cache_file(replay, dst_path, &dst_cache) == 0) {
					memset(path2, 0, PATH_MAX);
					sprintf(path2, "%s", dst_cache.c_str());
					isDstCache = 1;
				}
			}
		}
		file_rename(path, path2);
		if (isSrcCache)
			update_cache_file(replay, src_path, src_cache);
		if (isDstCache)
			update_cache_file(replay, dst_path, dst_cache);
	}
	else if (strncmp(type, "[WO]", 4) == 0)
	{
		long long int write_off;
		long long int write_size;
		long long int file_size;
		string str_path = string(path);
		string new_path;
		int isCacheFile = 0;

		if (str_path.find("unknown_") != std::string::npos){
			return -1;
		}
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		write_off = atoll(ptr);
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		write_size = atoll(ptr);
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		file_size = atoll(ptr);

		if (isCache) {
			if (test_and_create_cache(replay, str_path, file_size) == 0) {
				isCacheFile = 1;
			}
			else if (database_overwrite(replay, str_path, write_off, write_size, file_size) == 0){
				isCacheFile = 1;
			}
		}
		if (!isCacheFile)
			file_write(path, write_off, write_size, file_size);
	}
	else if (strncmp(type, "[WA]", 4) == 0)
	{
		long long int write_off;
		long long int write_size;
		long long int file_size;
		string str_path = string(path);
		string new_path;
		int isCacheFile = 0;

		if (str_path.find("unknown_") != std::string::npos){
			return -1;
		}
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		write_off = atoll(ptr);
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		write_size = atoll(ptr);
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		file_size = atoll(ptr);

		if (isCache) {
			if (test_and_create_cache(replay, str_path, file_size) == 0) {
				isCacheFile = 1;
			}
			else if (database_append(replay, str_path, write_off, write_size, file_size) == 0) {
				isCacheFile = 1;
			}
		}
		if (!isCacheFile)
			file_append(path, write_off, write_size, file_size);
	}
	else if (strncmp(type, "[TR]", 4) == 0)
	{
		long long int after_size;
		long long int before_size;
		int isCacheFile = 0;
		int DBType = -1;
		string str_path = string(path);
		string new_path;

		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		after_size = atoll(ptr);
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		before_size = atoll(ptr);

		if (isCache) {
			if (search_cache_file(replay, str_path, &new_path) == 0) {
				isCacheFile = 1;
			}
			else if (database_truncate(replay, str_path, after_size, before_size) == 0) {
				isCacheFile = 1;
			}
		}

		else if (!isCacheFile)
			file_truncate(path, after_size, before_size);
	}
	else if (strncmp(type, "[SL]", 4) == 0)
	{
		char path2[PATH_MAX + 1];
		struct stat stat_buf;
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		strncpy(tmp, ptr, strlen(ptr));
		tmp[strlen(ptr)] = 0x00;
		memset(path2, 0, PATH_MAX);
		sprintf(path2, "%s/%s", mount_dir, tmp);
		symlink(path, path2);
	}
	else if (strncmp(type, "[R]", 3) == 0)
	{
		long long int read_off;
		long long int read_size;
		long long int file_size;
		string new_path;
		int isCacheFile = 0;

		string str_path = string(path);
		if (str_path.find("unknown_") != std::string::npos){
			return -1;
		}
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		read_off = atoll(ptr);
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		read_size = atoll(ptr);
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			return -1;
		file_size = atoll(ptr);

		if (isCache && read_off == 0) {
			if (create_cache_file(replay, str_path) == 0) {
				isCacheFile = 1;	
			}
		}
		if (!isCacheFile)
			file_read(path, read_off, read_size, name);
	}
	else
		return -1;

	return last_time;
}

static double get_time(char *line)
{
	double last_time;
	char *ptr;
	char *ptr2;

	ptr = strtok(line, "\t");
	if (ptr == NULL)
		return -1;
	last_time = strtod(ptr, &ptr2);

	return last_time;
}

static FILE* pop_update(struct ReplayFile *load_replay, list<struct ReplayFile> *update_list)
{
	FILE* update_fp = NULL;

	while (1) 
	{
		if (!update_list->empty()) {
			struct ReplayFile update_replay = update_list->front();
			std::string load_name = std::string(load_replay->job->name);
			std::string update_name = std::string(update_replay.job->name);
			if (load_name.compare(update_name) == 0) {
				cout << load_name << ": SAME APP UPDATE" << endl;
				break;
			}
			update_list->pop_front();
			update_fp = fopen(update_replay.input_name.c_str(), "r");
			if (update_fp == NULL) {
				cout << "ERROR: No Exist: " << update_replay.input_name.c_str() << endl;
			}
			else {
				cout << "UPDATE: " << update_replay.input_name << endl;
				return update_fp;
			}
		}
		else
			break;
	}
	return NULL;
}

double do_trace_replay(struct ReplayFile *load_replay, list<struct ReplayFile> *update_list, char* name)
{
	double last_time = 0.0, cur_time = 0.0, sync_time = 0.0;
	char line_load[PATH_MAX], line_update[PATH_MAX];
	FILE *load_fp = NULL, *update_fp = NULL;
	bool updateTrace = false, loadTrace = false, loadEnd = false;
	bool firstUpdate = true;
	struct ReplayJob *replay;
	replay = load_replay->job;
	IG_lasttime = 0.0; 

	load_fp = fopen(load_replay->input_name.c_str(), "r");
	if (load_fp == NULL) {
		cout << "ERROR: No Exist: " << load_replay->input_name.c_str() << endl;
		return -1;
	}

	update_fp = pop_update(load_replay, update_list);
	last_time = cur_time;

	if (mode_flag & FSYNCTIME)
		print_starttrace_fsync(replay->curTime);

	while(1)
	{
		memset(line_load, 0, PATH_MAX);
		memset(line_update, 0, PATH_MAX);
		if (!loadTrace && !loadEnd) {
			if (fgets(line_load, PATH_MAX, load_fp) == NULL) {
				fclose(load_fp);
				loadTrace = false;
				loadEnd = true;
			}
			else
				loadTrace = true;
		}
		if (update_fp && !updateTrace)
		{
			if (fgets(line_update, PATH_MAX, update_fp) == NULL) {
				fclose(update_fp);
				if (loadEnd)
					break;
				update_fp = pop_update(load_replay, update_list);
				last_time = cur_time;
				firstUpdate = true;
				if (update_fp == NULL) {
					updateTrace = false;
				}

				continue;
			}
			updateTrace = true;
		}

		if (!loadTrace & !updateTrace)
			break;
		else if (updateTrace && loadTrace)
		{
			double load_time = get_time(line_load);
			double update_time = get_time(line_update);
			if (load_time == -1) {
				loadTrace = false;
				continue;
			} 
			if (update_time == -1) {
				updateTrace = false;
				continue;
			}
			if (update_time > 80000000)
			{
				__do_trace_replay(load_replay->mount_dir, line_update, replay, cur_time, 1);		
				updateTrace = false;
			}
			else 
			{
				if (firstUpdate) {
					last_time -= update_time;
					firstUpdate = false;
				}
				update_time += last_time;
				if (update_time < 0)
					cout << "minus" << endl;
				if (load_time > update_time) {
					__do_trace_replay(load_replay->mount_dir, line_update, replay, cur_time, 1);		
					updateTrace = false;
					cur_time = update_time;
				} else {
					__do_trace_replay(load_replay->mount_dir, line_load, replay, cur_time, 0);		
					loadTrace = false;
					cur_time = load_time;
				}
			}
		}
		else if (updateTrace) {
			__do_trace_replay(load_replay->mount_dir, line_update, replay, cur_time, 1);
			updateTrace = false;
		}
		else if(loadTrace) {
			__do_trace_replay(load_replay->mount_dir, line_load, replay, cur_time, 0);
			loadTrace = false;
		}
/*
		if (cur_time > (sync_time + 60))
		{
			sync_time = cur_time;
			sync();
		}
*/
	}
//	cout << "sync start:" << load_replay->input_name << endl;
	sync();
//	cout << "sync end:" << load_replay->input_name << endl;

	return last_time;
}

double do_trace_replay(char* mount_dir, char *input_name, struct ReplayJob *replay, double curTime, char* name)
{
	double last_time;
	char line[PATH_MAX];
	FILE *input_fp;
	struct trace_stat Stat;
	bool isCache = false;
	double sync_time = 0;
	bool firstTrace = false;

	if (strstr(input_name, ".input") == NULL)
		return -1;

	if (mode_flag & FSYNCTIME)
		print_starttrace_fsync(curTime);

	memset(&Stat, 0, sizeof(struct trace_stat));
	input_fp = fopen(input_name, "r");
	if (input_fp == NULL) {
		cout << "ERROR: No Exist: " << input_name << endl;
		return -1;
	}
	
	if (replay != NULL) {
		if (replay->type == REPLAY_LOADING)
			isCache = true;
	}
	printf("\n");
	memset(line, 0, PATH_MAX);
	while (fgets(line, PATH_MAX, input_fp) != NULL)
	{
		char *ptr;
		char *ptr_2;
		char type[10];
		char tmp[PATH_MAX + 1];
		char path[PATH_MAX + 1];
		string strLine(line);

		//cout << line << endl;

		ptr = strtok(line, "\t");
		if (ptr == NULL)
			continue;
		last_time = strtod(ptr, &ptr_2);
		if (firstTrace == false) {
			firstTrace = true;
			sync_time = last_time;
		}

		if (IG_mode == 1) {
			double itime = last_time - IG_lasttime;
			if (itime > 300)
				itime = 300;
			else if (itime < 0)
				itime = 1;
			IG_curTime += itime;
			IG_trace(strLine);
			IG_lasttime = last_time;
			continue;
		}
/*
		if (last_time > (sync_time + 60))
		{
			sync_time = last_time;
			sync();
		}
*/
		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			continue;
		strncpy(type, ptr, strlen(ptr));
		type[strlen(ptr)] = 0x00;

		ptr = strtok(NULL, "\t");
		if (ptr == NULL)
			continue;
		strncpy(tmp, ptr, strlen(ptr));
		tmp[strlen(ptr)] = 0x00;

		memset(path, 0, PATH_MAX);
		sprintf(path, "%s/data%s", mount_dir, tmp);

		if (strncmp(type, "[R]", 4) != 0) {
			if (replay != NULL) {
				if (replay->type == REPLAY_LOADING) {
					string str_path = string(path);
					if (str_path.find("/app/") != std::string::npos){
						continue;
					}
				}
			}
		}

		if (strncmp(type, "[CR]", 4) == 0) {
			string str_path = string(path);
			int isCreate = 1;
			if (isCache) {
				if (create_cache_file(replay, str_path) == 0) {
					isCreate = 0;
					Stat.create++;
				}
			}
/*
			if (isCreate) {
				if (file_create(path) == 0)
					Stat.create++;
			}
*/
		}
		else if (strncmp(type, "[MD]", 4) == 0) {
			if (file_mkdir(path) == 0)
				Stat.mkdir++;
		}
		else if (strncmp(type, "[UN]", 4) == 0) {
			int isUnlink = 1;
			if (isCache) {
				string new_path;
				string str_path = string(path);
				if (search_cache_file(replay, str_path, &new_path) == 0) {
					isUnlink = 0;
				}
				database_unlink(replay, string(path));
			}
			if (isUnlink == 1) {
				if (file_unlink(path) == 0)
					Stat.unlink++;
			}
		}
		else if (strncmp(type, "[RD]", 4) == 0) {
			string str_path = string(path);
			if (isCache) {
				if (test_cache(replay, str_path) == 0) {}
				else {
					if (file_rmdir(path) == 0)
						Stat.rmdir++;	
					remove_dir_caches(replay, str_path);
				}
			} else if (file_rmdir(path) == 0)
				Stat.rmdir++;	
		}
		else if (strncmp(type, "[FS]", 4) == 0)
		{
			int sync_option;
			string str_path = string(path);
			clock_t start_time, end_time;

			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			sync_option = atoi(ptr);
			fflush(stdout);

			if (isCache) {
				string new_path;
				if (search_cache_file(replay, str_path, &new_path) == 0) {
					memset(path, 0, PATH_MAX);
					sprintf(path, "%s", new_path.c_str());
				}
			}
			start_time = clock();
			if (file_fsync(path, sync_option) == 0)
				Stat.fsync++;
			end_time = clock();
			if (mode_flag & FSYNCTIME)
				print_time_fsync(start_time, end_time);
		}
		else if (strncmp(type, "[RN]", 4) == 0)
		{
			char path2[PATH_MAX + 1];
			struct stat stat_buf;
			int isSrcCache = 0, isDstCache = 0;
			string src_path, dst_path;
			string src_cache, dst_cache;

			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			strncpy(tmp, ptr, strlen(ptr));
			tmp[strlen(ptr)] = 0x00;
			memset(path2, 0, PATH_MAX);
			sprintf(path2, "%s/data/%s", mount_dir, tmp);

			src_path = string(path);
			dst_path = string(path2);	

			if ((stat(path, &stat_buf) >= 0) && isCache) 
			{
				if (S_ISDIR(stat_buf.st_mode)) {}
				else {
					if (search_cache_file(replay, src_path, &src_cache) == 0) {
						memset(path, 0, PATH_MAX);
						sprintf(path, "%s", src_cache.c_str());
						isSrcCache = 1;
					}
					if (search_cache_file(replay, dst_path, &dst_cache) == 0) {
						memset(path2, 0, PATH_MAX);
						sprintf(path2, "%s", dst_cache.c_str());
						isDstCache = 1;
					}
				}
			}

			if (file_rename(path, path2) == 0)
				Stat.rename++;

			if (isSrcCache)
				update_cache_file(replay, src_path, src_cache);
			if (isDstCache)
				update_cache_file(replay, dst_path, dst_cache);
		}
		else if (strncmp(type, "[WO]", 4) == 0)
		{
			long long int write_off;
			long long int write_size;
			long long int file_size;
			string str_path = string(path);
			int isCacheFile = 0;

			if (str_path.find("unknown_") != std::string::npos){
				continue;
			}

			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			write_off = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			write_size = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			file_size = atoll(ptr);

			if (isCache) {
				if (test_and_create_cache(replay, str_path, file_size) == 0) {
					isCacheFile = 1;
				}
				if (database_overwrite(replay, str_path, write_off, write_size, file_size) == 0){
					isCacheFile = 1;
				}

			}
			if (!isCacheFile) {
				if (file_write(path, write_off, write_size, file_size) == 0)
					Stat.write_overwrite++;
			}
		}
		else if (strncmp(type, "[WA]", 4) == 0)
		{
			long long int write_off;
			long long int write_size;
			long long int file_size;
			string str_path = string(path);
			int isCacheFile = 0;

			if (str_path.find("unknown_") != std::string::npos){
				continue;
			}

			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			write_off = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			write_size = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			file_size = atoll(ptr);

			if (isCache) {
				if (test_and_create_cache(replay, str_path, file_size) == 0) {
					isCacheFile = 1;
				}
				if (database_append(replay, str_path, write_off, write_size, file_size) == 0){
					isCacheFile = 1;
				}

			}
			if (!isCacheFile) {
				if (file_append(path, write_off, write_size, file_size) == 0)
					Stat.write_append++;
			}
		}
		else if (strncmp(type, "[TR]", 4) == 0)
		{
			long long int after_size;
			long long int before_size;
			string str_path = string(path);
			int isCacheFile = 0;
			string new_path; 

			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			after_size = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			before_size = atoll(ptr);

			if (isCache) {
				if (search_cache_file(replay, str_path, &new_path) == 0) {
					isCacheFile = 1;
				}
				else if (database_truncate(replay, str_path, after_size, before_size) == 0) {
					isCacheFile = 1;
				}
			}

			if (!isCacheFile)
				file_truncate(path, after_size, before_size);
		}
		else if (strncmp(type, "[SL]", 4) == 0)
		{
			char path2[PATH_MAX + 1];
			struct stat stat_buf;
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			strncpy(tmp, ptr, strlen(ptr));
			tmp[strlen(ptr)] = 0x00;
			memset(path2, 0, PATH_MAX);
			sprintf(path2, "%s/%s", mount_dir, tmp);
			symlink(path, path2);
		}
		else if (strncmp(type, "[R]", 3) == 0)
		{
			long long int read_off;
			long long int read_size;
			long long int file_size;
			string new_path;
			int isCacheFile = 0;

			string str_path = string(path);
			if (str_path.find("unknown_") != std::string::npos){
				continue;
			}
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			read_off = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			read_size = atoll(ptr);
			ptr = strtok(NULL, "\t");
			if (ptr == NULL)
				continue;
			file_size = atoll(ptr);

			if (isCache && read_off == 0) {
				if (create_cache_file(replay, str_path) == 0) {
					isCacheFile = 1;	
				}
			}
			if (!isCacheFile)
				file_read(path, read_off, read_size, name);
		}
		else
			continue;
	}

out:

/*	if (mode_flag & VERBOSE)
	{
		//printf("CR\tMD\tUN\tRD\tFS\tRN\tWO\tWA\n");
	
		printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", Stat.create, Stat.mkdir, 
									Stat.unlink, Stat.rmdir, 
									Stat.fsync, Stat.rename,
									Stat.write_overwrite, Stat.write_append);
		
	}
*/
	fclose(input_fp);

//	cout << "sync start:" << input_name << endl;
	sync();
//	cout << "sync end:" << input_name << endl;

	return last_time;
}

int file_create(const char *path)
{
	int new_fd = open (path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (new_fd < 0)
	{
		// printf("CR: Retry mkdir_all_path: %s\n", path);
		mkdir_all_path(path);
		new_fd = open (path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
		if (new_fd < 0) {
			if (errno == EEXIST) {
				return -1;
			}
		// 	printf("CR: Failed Create file: %s\n", path);
			return -1;
		}
	}
	write(new_fd, "a", 1);
	close(new_fd);
	truncate(path, 0);
	return 0;
}

int file_unlink(char *path)
{
	int ret;
	ret = unlink(path);
//	if (ret < 0)
//		printf("UN: Failed unlink file: %s\n", path);

	return 0;
}

int file_mkdir(char *path)
{
	int ret;
	mkdir_all_path(path);
	ret = mkdir(path, 0776);
	if (ret < 0) {
		if (errno == EEXIST) {
			struct stat buf;
			if (!stat(path, &buf))
			{
				if (S_ISREG(buf.st_mode))
				{
					unlink(path);
					ret = mkdir(path, 0776);
					if (ret == 0)
						return 0;
				}
			}
		}
		return -1;
//		printf("MD: Failed mkdir file: %s\n", path);
	}
	return 0;
}
int remove_directory(const char *path)
{
	DIR *d = opendir(path);
	size_t path_len = strlen(path);
	int r = -1;

	if (d)
	{
		struct dirent *p;
		r = 0;
		while (!r && (p=readdir(d)))
		{
			int r2 = -1;
			char buf[PATH_MAX];
			size_t len;
			/* Skip the names "." and ".." as we don't want to recurse on them. */
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
				continue;
			len = path_len + strlen(p->d_name) + 2; 
			struct stat statbuf;
			snprintf(buf, len, "%s/%s", path, p->d_name);
			if (!stat(buf, &statbuf))
			{
				if (S_ISDIR(statbuf.st_mode))
					r2 = remove_directory(buf);
				else
					r2 = unlink(buf);
			}
			r = r2;
		}
		closedir(d);
	}
	if (!r)
		r = rmdir(path);

	return r;
}
int file_rmdir(char *path)
{
	int ret = 0;
	char cmd[PATH_MAX];
	remove_directory(path);
	//ret = rmdir(path);
	return 0;
}

int file_fsync(char *path, int option)
{
	int fd = open(path, O_RDWR, 0644);
	if (fd < 0)
	{
		if (errno == EISDIR)
			return -1;
//		printf("FS: Fail Open: %s\n", path);
		return -1;
	}
	if (option == 1)
		fsync(fd);
	else
		fdatasync(fd);

	close(fd);
	return 0;
}

int file_rename(char *path1, char *path2)
{
	struct stat stat_buf;
	int ret = rename(path1, path2);
	if (ret < 0)
	{
		stat(path2, &stat_buf);
		if(S_ISDIR(stat_buf.st_mode))
			file_rmdir(path2);
		else
			file_unlink(path2);
		ret = rename(path1, path2);
		if (ret < 0) {
			ret = file_create(path2);
		}
	}
	return ret;
}

int file_write(const char *path, long long int write_off, 
				long long int write_size, long long int file_size)
{
	int fd = open(path, O_RDWR);
	char *data;
	char letter = 'a';
	int letter_count = 'z'- 'a';
	int random_number;
	if (fd < 0) {
		file_create(path);
		fd = open(path, O_RDWR);
		if (fd < 0) {
		// 	printf("W: Failed Open:%s\n", path);
			return -1;
		}
	}
	data = (char*) malloc(write_size);
	random_number = rand() % letter_count;
	letter = letter + random_number;
	memset(data, letter, write_size);
	lseek(fd, write_off, SEEK_SET);
	write(fd, data, write_size);
	close(fd);
	free(data);
	return 0;
}

int file_append(const char *path, long long int write_off, 
				long long int write_size, long long int file_size)
{
	int fd = open(path, O_RDWR);
	char *data;
	char letter = 'a';
	int letter_count = 'z'- 'a';
	int random_number;
	int temp_file = 0;
	unsigned long long int offset = 0;	

	if (fd < 0) {
		// printf("W: Failed Open, Try file create:%s\n", path);
		file_create(path);
		fd = open(path, O_RDWR);
		if (fd < 0) {
			// printf("W: Failed Open:%s\n", path);
			return -1;
		}
	}

	if (file_size == 0) {
		truncate(path, 0);
	}
	if (strstr(path, "-journal") != NULL)
		temp_file = 1;
	else if (strstr(path, "-wal") != NULL)
		temp_file = 1;
//	else if (strstr(path, "-shm") != NULL)
//		temp_file = 1;

	data = (char *)malloc(write_size);
	random_number = rand() % letter_count;
	letter = letter + random_number;
	memset(data, letter, write_size);
	offset = lseek(fd, 0, SEEK_END);
	if ((temp_file == 1) && (offset > 33554432)) {
		lseek(fd, write_off, SEEK_SET);
	} else if (offset > 1073741824)
		lseek(fd, write_off, SEEK_SET);

	write(fd, data, write_size);
	close(fd);
	free(data);
	return 0;
}

int file_truncate(const char *path,
				long long int after_size, long long int before_size)
{
	int fd = open(path, O_RDWR);
	struct stat file_stat;
	long long int file_size;
	long long int target_size;
	int ret;

	if (fd < 0)
		return -1;

	ret = fstat(fd, &file_stat);
	if  (ret < 0) {
		cout << path << ": fstat error" << endl;
		return -1;
	}

	if (!S_ISREG(file_stat.st_mode)) {
		close(fd);
		return -1;
	}

	if (after_size == 0)
		target_size = 0;
	else 
	{
		file_size = file_stat.st_size;
		target_size = file_size - (before_size - after_size);
		if (target_size < 0)
			target_size = 0;
	}
	
	ret = ftruncate(fd, target_size);
	if (ret < 0) {
		cout << path << ": ftruncate error:" << ret << endl;
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int file_truncate_same(const char *path,
				long long int after_size, long long int before_size)
{
	int fd = open(path, O_RDWR);
	struct stat file_stat;
	long long int file_size;
	long long int target_size;
	int ret;

	if (fd < 0)
		return -1;

	ret = fstat(fd, &file_stat);
	if  (ret < 0) {
		cout << path << ": fstat error" << endl;
		return -1;
	}

	if (!S_ISREG(file_stat.st_mode)) {
		close(fd);
		return -1;
	}

	target_size = after_size;

	ret = ftruncate(fd, target_size);
	if (ret < 0) {
		cout << path << ": ftruncate error:" << ret << endl;
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}



int file_read(char *path, long long int read_off, long long int read_size, char *name)
{
	int fd;
	char *data;
	int random_number;
	data = (char*) malloc(read_size);
	random_number = rand() % 10;
	clock_t start_time, end_time; //update

	if (random_number < 7)
		fd = open(path, O_DIRECT);
	else
		fd = open(path, O_RDWR);

	if (fd < 0) {
		free(data);
		return -1;
	}

	if (lseek(fd, read_off, SEEK_SET) < 0) {
		close(fd);
		free(data);
		return -1; 
	}

	//cout << "path: " << path << endl;
	start_time = clock(); //update
	read(fd, data, read_size);
	end_time = clock();		//update
	close(fd);
	free(data);
	print_time_read(name, start_time, end_time); // update
	return 0;
}


int IG_trace(string strLine)
{
	FILE *fOut = fopen(IG_outPath.c_str(), "a");
	char outText[500];

	sprintf(outText, "%lf\t%s", IG_curTime, strLine.c_str());
//	cout << outText;
	fputs(outText, fOut);
	fclose(fOut);

	return 0;
}

long long int get_file_size(string path)
{
	struct stat file_info;
	unsigned long long int size;
	if (stat(path.c_str(), &file_info) < 0)
		return -1;
	size = file_info.st_size;
	return (size + 4095) / 4096;
}

static void evict_cache_files(struct CacheDirInfo *cacheDirInfo)
{
	unsigned long long target_size = cacheDirInfo->max_cache_size - cacheDirInfo->max_cache_size * cacheDirInfo->evict_ratio;
	unsigned long long cur_size = cacheDirInfo->cur_cache_size;

	while (target_size < cur_size) {
		char path[PATH_MAX + 1];
		string del_path = cacheDirInfo->lru_list.front();
		string trace_path;
		unsigned long long size;

		// cout << cacheDirInfo->lru_list.size() << " "  << del_path << endl;
		cacheDirInfo->lru_list.pop_front();

		if (cacheDirInfo->file_smap.find(del_path) == cacheDirInfo->file_smap.end()) {
			//size = 0;
		} else  {
			size = cacheDirInfo->file_smap.at(del_path);
			cacheDirInfo->file_smap.erase(del_path);
		}
		memset(path, 0, PATH_MAX);
		sprintf(path, "%s", del_path.c_str());

//		cout << del_path << ":" << size << endl;		

		file_unlink(path);
		cur_size -= size;

		if (cacheDirInfo->file_rmap.find(del_path) == cacheDirInfo->file_rmap.end()) {
			continue;
		}  
		trace_path = cacheDirInfo->file_rmap.at(del_path);
		cacheDirInfo->file_rmap.erase(del_path);

		if (cacheDirInfo->file_map.find(trace_path) == cacheDirInfo->file_map.end()) {
			continue;
		}  
		if (del_path == cacheDirInfo->file_map.at(trace_path))
			cacheDirInfo->file_map.erase(trace_path);

//		cout << "Cache Eviction: " << path << " size:" << size << endl;
	//	cout << "[SIZE]: " << cur_size << " target:" << target_size << endl;
	}
	cacheDirInfo->cur_cache_size = cur_size;
}
#define BYTE_TO_BLOCK(size) ((size + 4095) / 4096)
static string get_clear_path(string trace_path) 
{
	string path = trace_path;
	while (1) {
		int found = path.find("//");
		if (found == std::string::npos)
			break; 
		path.replace(found, 2, "/");	
	}
	return path;
}

static void remove_dir_caches(struct ReplayJob *arg_replay, string trace_path)
{
	int idx; 
	struct ReplayJob *replay;

	if (arg_replay->type == REPLAY_LOADING)
		replay = arg_replay;
	else if (arg_replay->loadJob != NULL)
		replay = arg_replay->loadJob;
	else
		return;

	for (idx = 0; idx < replay->cache.cache_list.size(); idx++) 
	{
		struct CacheDirInfo *cacheDirInfo = &(replay->cache.cache_list[idx]);
		map<string, string>::iterator rmap_it = cacheDirInfo->file_rmap.begin();
		string cache_path;
		string search_path = get_clear_path(trace_path);

		if (rmap_it == cacheDirInfo->file_rmap.end()) {
			continue;
		}
		
		cache_path = rmap_it->first;
		if (cache_path.find(search_path) == std::string::npos)
			continue;

		cacheDirInfo->cur_cache_size = 0;
		cacheDirInfo->file_map.clear();
		cacheDirInfo->file_rmap.clear();
		cacheDirInfo->file_smap.clear();
		cacheDirInfo->lru_list.clear();

//		cout << "remove_dir_caches:" << search_path << " " << cache_path << endl;
	}
}


static int init_cache_file(struct CacheDirInfo *cacheDirInfo, string trace_path, unsigned long long int size)
{
	map<string, string>::iterator map_it;
	string path = get_clear_path(trace_path);

	// file map update
	map_it = cacheDirInfo->file_map.find(path);
	if (map_it != cacheDirInfo->file_map.end()) {
//		map_it->second = path;
		cout << "ERROR:" << path << " samefiles" << endl;
		exit(1);
	}
	cacheDirInfo->file_map.insert(pair<string, string>(path, path));
	cacheDirInfo->file_rmap.insert(pair<string, string>(path, path));
	cacheDirInfo->file_smap.insert(pair<string, unsigned long long>(path, BYTE_TO_BLOCK(size)));

	if (cacheDirInfo->file_map.size() > 20000)
		cout << "WARNING: file_map size:" << cacheDirInfo->file_map.size() << " " << cacheDirInfo->path << endl;
	if (cacheDirInfo->file_rmap.size() > 20000)
		cout << "WARNING: file_rmap size:" << cacheDirInfo->file_rmap.size() << " " << cacheDirInfo->path << endl;
	if (cacheDirInfo->file_smap.size() > 20000)
		cout << "WARNING: file_smap size:" << cacheDirInfo->file_smap.size() << " " << cacheDirInfo->path << endl;

	// lru update
	cacheDirInfo->lru_list.push_back(path);

	if (cacheDirInfo->lru_list.size() > 20000)
		cout << "WARNING: lru_list size:" << cacheDirInfo->lru_list.size() << " " << cacheDirInfo->path << endl;


	cacheDirInfo->cur_cache_size += BYTE_TO_BLOCK(size);
	return 0;
}

static struct CacheDirInfo* get_cache_info(struct ReplayJob *replay, string path)
{
	int idx;
	string trace_path = get_clear_path(path);

	if ((trace_path.find("cache") == std::string::npos) &&
					(trace_path.find("files") == std::string::npos))
		return NULL;

	if ((trace_path.find("index") != std::string::npos) ||
					(trace_path.find("journal") != std::string::npos) ||
					(trace_path.find("sqlite") != std::string::npos))
		return NULL;

	for (idx = 0; idx < replay->cache.cache_list.size(); idx++) 
	{
		std::size_t found = trace_path.find(replay->cache.cache_list[idx].path);

		if (found != std::string::npos) {
			struct CacheDirInfo *cacheDirInfo = &(replay->cache.cache_list[idx]);
			return cacheDirInfo;
		}
	}
	return NULL;
}

static string get_dir_path(string full_path)
{
	int found;
	string path = get_clear_path(full_path);
	string dir_path;

	found = path.rfind("/");
	if (found == std::string::npos) {
		cout << "get_dir_path(error): " << full_path << endl;
		return string("UNKNOWN");
	}
	
	dir_path = path.substr(0, found);

	return dir_path;
}

static struct CacheFileInfo* check_access_cache(struct CacheDirInfo* cacheDirInfo)
{
	int reuse = false;
	int fileID = -1;
	struct CacheFileInfo* fileinfo = NULL;

	if (select_unique(cacheDirInfo->unique_ratio) || (cacheDirInfo->cacheref.filelist.size() >= 10000)) {
		fileinfo = try_reuse(&cacheDirInfo->cacheref);
	}
	if (fileinfo == NULL) {
		int filesize = select_filesize(&cacheDirInfo->sizevec);
		int ref;
		if (cacheDirInfo->c == 0)
			cacheDirInfo->c = calc_c(cacheDirInfo->max_ref);

		ref = select_ref(cacheDirInfo->onetimes_ratio, 
						cacheDirInfo->c,
						cacheDirInfo->zipf_slope,
						cacheDirInfo->max_ref);

		fileinfo = new CacheFileInfo;

		fileinfo->fileID = cacheDirInfo->idcount; 
		fileinfo->filesize = filesize;
		fileinfo->ref = ref;

		if (ref > 0) {
			cacheDirInfo->cacheref.filelist.push_back(fileinfo);
			cacheDirInfo->cacheref.total_ref += ref;
			if (cacheDirInfo->cacheref.filelist.size() >= 20000) 
				cout << "filelist size:" << cacheDirInfo->cacheref.filelist.size() << " path:" << cacheDirInfo->path << endl;
		}
		cacheDirInfo->idcount++;
	}
	return fileinfo;  
}

int get_extension(string path, string *ext)
{
	std::size_t ext_idx;
	std::size_t dir_idx;
	int found = 0;

	ext_idx = path.rfind("."); 
	dir_idx = path.rfind("/");

	if(ext_idx == std::string::npos)
		return -1;

	if(dir_idx == std::string::npos)
		found = 1;
	else {
		if (dir_idx < ext_idx) 
			found = 1;
	}

	if (found == 1) {
		*ext = path.substr(ext_idx+1);
		return 0;
	}
	return -1;
}

static int create_cache_file(struct ReplayJob *arg_replay, string path)
{
	struct CacheDirInfo* cacheDirInfo;
	string new_path, str_ext;
	char str_path[PATH_MAX];
	map<string, string>::iterator map_it;
	string trace_path = get_clear_path(path);
	int fileID;
	struct CacheFileInfo *fileinfo;
	long long int filesize;
	struct ReplayJob *replay;

	if (arg_replay->type == REPLAY_LOADING)
		replay = arg_replay;
	else if (arg_replay->loadJob != NULL)
		replay = arg_replay->loadJob;
	else
		return -1;

	cacheDirInfo = get_cache_info(replay, trace_path); 
	if (cacheDirInfo == NULL)
		return -1;

	fileinfo = check_access_cache(cacheDirInfo);
	if (fileinfo == NULL)
		return 0;
	
	if (get_extension(trace_path, &str_ext) == 0) 
		sprintf(str_path , "%s/%d.%s", get_dir_path(trace_path).c_str(), fileinfo->fileID, str_ext.c_str());
	else 
		sprintf(str_path , "%s/%d", get_dir_path(trace_path).c_str(), fileinfo->fileID);

	new_path = string(str_path);
	new_path = get_clear_path(new_path);

	filesize = fileinfo->filesize;

	if (fileinfo->ref == 0)
		delete fileinfo;

	map_it = cacheDirInfo->file_rmap.find(new_path);
	if (map_it != cacheDirInfo->file_rmap.end())
		return 0;

	// Create
	if (cacheDirInfo->cur_cache_size > cacheDirInfo->max_cache_size)
		evict_cache_files(cacheDirInfo); 

//	cout << "Create Cache: " << str_path << " " << fileinfo->filesize << endl;

	// file map update
	map_it = cacheDirInfo->file_map.find(trace_path);
	if (map_it != cacheDirInfo->file_map.end()) {
		map_it->second = new_path;		
	}
	else {
		cacheDirInfo->file_map.insert(pair<string, string>(trace_path, new_path));
	}
	cacheDirInfo->file_rmap.insert(pair<string, string>(new_path, trace_path));
	cacheDirInfo->file_smap.insert(pair<string, unsigned long long>(new_path, 0));

	if (cacheDirInfo->file_map.size() > 20000)
		cout << "WARNING: file_map size:" << cacheDirInfo->file_map.size() << " " << cacheDirInfo->path << endl;
	if (cacheDirInfo->file_rmap.size() > 20000)
		cout << "WARNING: file_rmap size:" << cacheDirInfo->file_rmap.size() << " " << cacheDirInfo->path << endl;
	if (cacheDirInfo->file_smap.size() > 20000)
		cout << "WARNING: file_smap size:" << cacheDirInfo->file_smap.size() << " " << cacheDirInfo->path << endl;

	// lru update
	cacheDirInfo->lru_list.push_back(new_path);
	file_create(str_path);

	file_append(str_path, 0, filesize, filesize);
//	cout << "[CR]" << trace_path << "->" << new_path << " " << endl;
	update_cache_file(replay, trace_path, new_path);

	insert_recent_cache(cacheDirInfo, path);

	return 0;
}

static int search_recent_cache(struct CacheDirInfo *cacheDirInfo, string trace_path)
{
	int idx;
	list<CacheSize>::iterator it;

	for (it = cacheDirInfo->recent_cache.begin(); it != cacheDirInfo->recent_cache.end(); it++)
	{
		struct CacheSize cache = (*it);
		std::size_t found = trace_path.find(cache.path);
		if (found != std::string::npos) {
			return 0;			
		}
	}
	return -1;
}

static int insert_recent_cache(struct CacheDirInfo *cacheDirInfo, string trace_path)
{
	int idx;
	struct CacheSize cache;
	cache.path = trace_path;
	cache.filesize = 0;

	cacheDirInfo->recent_cache.push_back(cache);
	if (cacheDirInfo->recent_cache.size() > 5) {
		cache = cacheDirInfo->recent_cache.front();
		cacheDirInfo->recent_cache.pop_front();
		if (cache.filesize > 0) {
			if (cacheDirInfo->sizevec.size() < 100) {
				cacheDirInfo->sizevec.push_back(cache.filesize);
			}
		}
	}

	return 0; 
}

static int update_recent_cache(struct CacheDirInfo *cacheDirInfo, string trace_path, long long int filesize)
{
	if (cacheDirInfo->sizevec.size() >= 100)
		return 0;

	list<CacheSize>::iterator it;

	for (it = cacheDirInfo->recent_cache.begin(); it != cacheDirInfo->recent_cache.end(); it++)
	{
		struct CacheSize *cache = &(*it);
		std::size_t found = trace_path.find(cache->path);
		if (found != std::string::npos) {
			cache->filesize = filesize;
			return 0;
		}
	}
	return 0;
}

static int test_cache(struct ReplayJob *arg_replay, string trace_path)
{
	struct ReplayJob *replay;
	struct CacheDirInfo *cacheDirInfo;

	if (arg_replay->type == REPLAY_LOADING)
		replay = arg_replay;
	else if (arg_replay->loadJob != NULL)
		replay = arg_replay->loadJob;
	else
		return -1;

	cacheDirInfo = get_cache_info(replay, trace_path);
	if (cacheDirInfo == NULL)
		return -1;

	return 0;
}

static int test_and_create_cache(struct ReplayJob *arg_replay, string trace_path, long long int file_size)
{
	struct ReplayJob *replay;
	struct CacheDirInfo *cacheDirInfo;

	if (arg_replay->type == REPLAY_LOADING)
		replay = arg_replay;
	else if (arg_replay->loadJob != NULL)
		replay = arg_replay->loadJob;
	else
		return -1;

	cacheDirInfo = get_cache_info(replay, trace_path);
	if (cacheDirInfo == NULL)
		return -1;

	if (search_recent_cache(cacheDirInfo, trace_path) == 0) {
		update_recent_cache(cacheDirInfo, trace_path, file_size);
		return 0;
	}

	create_cache_file(replay, trace_path);
	return 0;
}

static int search_cache_file(struct ReplayJob *arg_replay, string trace_path, string *new_path)
{
	int idx;
	struct ReplayJob *replay;

	if (arg_replay->type == REPLAY_LOADING)
		replay = arg_replay;
	else if (arg_replay->loadJob != NULL)
		replay = arg_replay->loadJob;
	else
		return -1;

	for (idx = 0; idx < replay->cache.cache_list.size(); idx++) 
	{
		string keyword = replay->cache.cache_list[idx].path;
		std::size_t found = trace_path.find(keyword);
		if (found != std::string::npos) {
			struct CacheDirInfo *cacheDirInfo = &(replay->cache.cache_list[idx]);
			map<string, string>::iterator map_it;
			map_it = cacheDirInfo->file_map.find(trace_path);
			if (map_it != cacheDirInfo->file_map.end()) {
				*new_path = map_it->second;
				return 0;
			} else {
				*new_path = get_clear_path(trace_path);
				return update_cache_file(replay, trace_path, *new_path);
			}
		}
	}
	return -1;
}

static int update_cache_file(struct ReplayJob *arg_replay, string trace_path, string new_path)
{
	struct CacheDirInfo *cacheDirInfo;
	map<string, unsigned long long>::iterator smap_it;
	long long int size;
	int ret = 0;
	struct ReplayJob *replay;

	if (arg_replay->type == REPLAY_LOADING)
		replay = arg_replay;
	else if (arg_replay->loadJob != NULL)
		replay = arg_replay->loadJob;
	else
		return -1;

	cacheDirInfo = get_cache_info(replay, trace_path);
	if (cacheDirInfo == NULL)
		return -1;

	trace_path = get_clear_path(trace_path);
	new_path = get_clear_path(new_path);
	size = get_file_size(new_path);
	smap_it = cacheDirInfo->file_smap.find(new_path);

	if (size == -1)
		ret = -1;

	if (smap_it != cacheDirInfo->file_smap.end()) {
		long long int old_size = cacheDirInfo->file_smap.at(new_path);
		if (size == -1) {
			std::list<string>::iterator lru_it;
			cacheDirInfo->file_rmap.erase(new_path);	
			cacheDirInfo->file_smap.erase(new_path);
			cacheDirInfo->lru_list.remove(new_path);

			if (cacheDirInfo->file_map.find(trace_path) != cacheDirInfo->file_map.end()) {
				if (new_path == cacheDirInfo->file_map.at(trace_path))
					cacheDirInfo->file_map.erase(trace_path);
			}

			if (old_size > 0) {
				if (old_size > cacheDirInfo->cur_cache_size) {
					cout << "ERROR: " << old_size << " " << trace_path << endl;
					cacheDirInfo->cur_cache_size = 0;
					exit(1);
				}
				else
					cacheDirInfo->cur_cache_size -= old_size;
			}
//			cout << "[UP-SIZE](delete):" << trace_path << " " << new_path << " oldsize:" << old_size << endl;
		} else if (old_size == size) {
			return 0;
		}
		else {
			cacheDirInfo->file_smap[new_path] = size;
			if (old_size >= 0) {
				if (size > old_size) 
					cacheDirInfo->cur_cache_size += (size - old_size);
				else {
					cacheDirInfo->cur_cache_size -= (old_size - size);
				}
			}
//			cout << "[UP-SIZE] (update):" << trace_path << " " << new_path << " oldsize:" << old_size << " size:" << size <<  endl;
		}
	} else {
		if (size >= 0) {
			map<string, string>::iterator map_it;
		    map_it = cacheDirInfo->file_map.find(trace_path);
		    if (map_it == cacheDirInfo->file_map.end()) {
				cacheDirInfo->file_map.insert(pair<string, string>(trace_path, new_path));
		    }
		    cacheDirInfo->file_rmap.insert(pair<string, string>(new_path, trace_path));
		    cacheDirInfo->file_smap.insert(pair<string, unsigned long long>(new_path, size));
		    cacheDirInfo->lru_list.push_back(new_path);
		    cacheDirInfo->cur_cache_size += size;
			if (cacheDirInfo->file_map.size() > 20000)
				cout << "WARNING: file_map size:" << cacheDirInfo->file_map.size() << " " << cacheDirInfo->path << endl;
			if (cacheDirInfo->file_rmap.size() > 20000)
				cout << "WARNING: file_rmap size:" << cacheDirInfo->file_rmap.size() << " " << cacheDirInfo->path << endl;
			if (cacheDirInfo->file_smap.size() > 20000)
				cout << "WARNING: file_smap size:" << cacheDirInfo->file_smap.size() << " " << cacheDirInfo->path << endl;


//			cout << "[UP-SIZE] (new):" << trace_path << " " << new_path << " size:" << size <<  endl;
		}
		else {
//			cout << "[UP-SIZE] (no size):" << trace_path << " " << new_path << endl;
		}
	}
	return ret;
}

int init_cache(char* mount_dir, const char *prefix, struct CacheDirInfo *cacheDirInfo)
{
	char dir_path[PATH_MAX + 1];
	DIR *cache_dir;
	struct dirent *dentry;
	
	memset(dir_path, 0, PATH_MAX);
	sprintf(dir_path, "%s/%s/%s/", mount_dir, prefix, cacheDirInfo->path.c_str());
	cache_dir = opendir(dir_path);

	if (cache_dir == NULL) { 
//		cout << dir_path << ": failed(init_cache)" << endl;
		return -1;
	}

	while (dentry = readdir(cache_dir)) {
		string entry_path;
		string full_path;
		unsigned long long int size;
		if (dentry->d_type != DT_REG)
			continue;
		entry_path = string(dentry->d_name);
		full_path = string(dir_path) + entry_path;
		size = get_file_size(full_path);
		init_cache_file(cacheDirInfo, full_path, size);
	}

	closedir(cache_dir);

	return 0;
}

static struct DBInfo* search_db_info(struct ReplayJob *arg_replay, string trace_path)
{
	map<string, struct DBInfo*>::iterator db_it;
	struct DBInfo *dbinfo;
	struct ReplayJob *replay;

	if (arg_replay->type == REPLAY_LOADING)
		replay = arg_replay;
	else if (arg_replay->loadJob != NULL)
		replay = arg_replay->loadJob;
	else
		return NULL;

	if ((trace_path.find("/databases/") == std::string::npos) &&
					(trace_path.find(".db") == std::string::npos))
		return NULL;

	trace_path = get_clear_path(trace_path);

	db_it = replay->db.db_map.find(trace_path);
	if (db_it == replay->db.db_map.end())
		return NULL;

	dbinfo = db_it->second;
	return dbinfo;
}

static int search_db_type(struct ReplayJob *arg_replay, string trace_path)
{
	struct DBInfo *dbinfo = search_db_info(arg_replay, trace_path); 
	if (dbinfo == NULL)
		return -1;
	
	return dbinfo->type;
}

static int database_overwrite(struct ReplayJob *replay, string trace_path, long long int write_off, long long int write_size, long long int file_size)
{
	struct DBInfo *dbinfo = search_db_info(replay, trace_path); 
	if (dbinfo == NULL)
		return -1;

	if(dbinfo->type == DBTYPE_INSERT) {
		if ((write_off >= (dbinfo->meta_offset * 4096))) {
			int start_off = write_off / 4096;
			int end_off = (write_off + write_size - 1) / 4096;
			int hot_len = dbinfo->hot_list.size();
			int warm_len = dbinfo->warm_list.size();
			int select = DBTEMP_COLD;

			if (hot_len == 0 && warm_len == 0) {
				file_write(trace_path.c_str(), write_off, write_size, file_size); 
				return 0; 
			}
		
			if (hot_len == 0)
				select = DBTEMP_WARM;
			else if (warm_len == 0)
				select = DBTEMP_HOT;

			for (int i = start_off; i <= end_off; i++) {
				int temp = select;
				if (select == DBTEMP_COLD) {
					long long int rand_off = rand() % 100;
					if (rand_off < dbinfo->hot_wrate)
						temp = DBTEMP_HOT;
					else
						temp = DBTEMP_WARM;
				}

				if (temp == DBTEMP_HOT) {
					long long int rand_off = rand() % hot_len;
					write_off = dbinfo->hot_list[rand_off]*4096;
				}
				else {
					long long int rand_off = rand() % warm_len;
					write_off = dbinfo->warm_list[rand_off]*4096;
				}
				if (write_size < 4096) {
					file_write(trace_path.c_str(), write_off, write_size, file_size); 
				} else {
					file_write(trace_path.c_str(), write_off, 4096, file_size);
				}
			}
		}
	} else 
		file_write(trace_path.c_str(), write_off, write_size, file_size); 
	return 0; 
}

static int database_append(struct ReplayJob *replay, string trace_path, long long int write_off, long long int write_size, long long int file_size)
{
	struct DBInfo *dbinfo = search_db_info(replay, trace_path); 
	if (dbinfo == NULL)
		return -1;

	if((dbinfo->type == DBTYPE_FIXED)) {
	} 
	else {
		if(dbinfo->type == DBTYPE_INSERT) {
			int start_off = write_off / 4096;
			int end_off = (write_off + write_size - 1) / 4096;
			
			for (int i = start_off; i <= end_off; i++) {
				long long int rand_off = rand() % 100;

				if (dbinfo->block_type.find(i) != dbinfo->block_type.end()) {
					if (dbinfo->block_type[i] != DBTEMP_COLD)
						continue;
				}

				if (rand_off < dbinfo->hot_brate) {
					dbinfo->hot_list.push_back(i);
					dbinfo->block_type[i] = DBTEMP_HOT;
				} else if (rand_off < dbinfo->hot_brate + dbinfo->cold_brate) {
					dbinfo->block_type[i] = DBTEMP_COLD;
				} else {
					dbinfo->warm_list.push_back(i);
					dbinfo->block_type[i] = DBTEMP_WARM;
				}
			}
		} 
		file_append(trace_path.c_str(), write_off, write_size, file_size); 

// file limit
		if (dbinfo->type == DBTYPE_INSERT) {
			struct stat stat_buf;
			if (stat(trace_path.c_str(), &stat_buf) >= 0) {
				int real_filesize = (stat_buf.st_size + 4095) / 4096;
				if (real_filesize > dbinfo->limit_size) {
					file_truncate_same(trace_path.c_str(), real_filesize * 9 / 10, real_filesize);	
					cout << "DB TR: " << trace_path << " " << real_filesize << endl;
				}
			}
		}
	}
	return 0; 
}

static int database_truncate(struct ReplayJob *replay, string trace_path, long long int after_size, long long int before_size)
{
	struct DBInfo *dbinfo = search_db_info(replay, trace_path); 
	if (dbinfo == NULL)
		return -1;

	if (dbinfo->type != DBTYPE_INSERT) {
		file_truncate_same(trace_path.c_str(), after_size, before_size);
	} else {
		struct stat stat_buf;
		if (stat(trace_path.c_str(), &stat_buf) >= 0) {
			long long int tr_size = before_size - after_size;
			long long int delete_start = (stat_buf.st_size-tr_size) / 4096; 
			int i;
			int hot_len = dbinfo->hot_list.size();
			int warm_len = dbinfo->warm_list.size(); 

			if (after_size == 0)
				delete_start = 0;

			for (i = hot_len-1; i >= 0; i--) {
				if (dbinfo->hot_list[i] >= delete_start) {
					dbinfo->block_type[dbinfo->hot_list[i]] = DBTEMP_COLD;
					dbinfo->hot_list.pop_back(); 
				}
				else
					break;
			}

			for (i = warm_len-1; i >= 0; i--) {
				if (dbinfo->warm_list[i] >= delete_start) {
					dbinfo->block_type[dbinfo->warm_list[i]] = DBTEMP_COLD;
					dbinfo->warm_list.pop_back(); 
				}
				else
					break;
			}
		}
		file_truncate(trace_path.c_str(), after_size, before_size);
	}

	return 0;
}

static int database_unlink(struct ReplayJob *replay, string trace_path)
{
	struct DBInfo *dbinfo = search_db_info(replay, trace_path); 
	if (dbinfo == NULL)
		return -1;

	if (dbinfo->type == DBTYPE_INSERT) {
		long long int delete_start = 0;
		int i;
		int hot_len = dbinfo->hot_list.size();
		int warm_len = dbinfo->warm_list.size(); 

		for (i = hot_len-1; i >= 0; i--) {
			if (dbinfo->hot_list[i] >= delete_start) {
				dbinfo->block_type[dbinfo->hot_list[i]] = DBTEMP_COLD;
				dbinfo->hot_list.pop_back(); 
			}
			else
				break;
		}

		for (i = warm_len-1; i >= 0; i--) {
			if (dbinfo->warm_list[i] >= delete_start) {
				dbinfo->block_type[dbinfo->warm_list[i]] = DBTEMP_COLD;
				dbinfo->warm_list.pop_back(); 
			}
			else
				break;
		}
	}
	return 0;
}

/*
int check_cache_file(string path) {
	if ((str_path.find("/cache/") != std::string::npos) || (str_path.find("/file/") != std::string::npos))
	{
		int idx;
		for (idx = 0; idx < replay->cache.cache_list.size(); idx++)
		{
			std::size_t found = str_path.find(replay->cache.cache_list[idx].path);
			if (found != std::string::npos)
			{
				double mod_lifespan = curTime+(replay->cache.cache_list[idx].lifespan);
				map<string, double>::iterator map_it;
				map_it = replay->cache.cache_file.find(str_path);
				if (map_it != replay->cache.cache_file.end())
					map_it->second = mod_lifespan;
				else
					replay->cache.cache_file.insert
						(pair<string, double>(str_path, mod_lifespan));
				break;
				}
			}
	}
	return 0;
}
*/

int print_starttrace_fsync(double curTime)
{
	FILE *fOut = fopen("result/fsynctime", "a");
	char outText[500];
	sprintf(outText, "T %lf\n", curTime);
	fputs(outText, fOut);
	fclose(fOut);
}

int print_time_fsync(clock_t start_time, clock_t end_time)
{
	FILE *fOut = fopen("result/fsynctime", "a");
	char outText[500];
	sprintf(outText, "F %lf\n", (double)(end_time-start_time)/(CLOCKS_PER_SEC));
	fputs(outText, fOut);
	fclose(fOut);
} 

int print_time_read(char* name, clock_t start_time, clock_t end_time) // update
{
	if (name == NULL)
		return 1;
	if (!strcmp(name, "airbnb") || !strcmp(name, "aliexpress") 
			|| !strcmp(name, "amazon") || !strcmp(name, "angrybirds") 
			|| !strcmp(name, "chrome") || !strcmp(name, "clash")
			|| !strcmp(name, "cnn") || !strcmp(name, "ebay") 
			|| !strcmp(name, "facebook") || !strcmp(name, "firefox") 
			|| !strcmp(name, "flipboard") || !strcmp(name, "gmail")
			|| !strcmp(name, "googlemap") || !strcmp(name, "instagram") 
			|| !strcmp(name, "melon") || !strcmp(name, "subways") 
			|| !strcmp(name, "trivago") || !strcmp(name, "twitter")
			|| !strcmp(name, "youtube")) {
		char buf[PATH_MAX];
		sprintf(buf, "readtime/%s", name);
		FILE *fOut = fopen(buf, "a");
		char outText[500];
		sprintf(outText, "%ld\n", end_time - start_time);
		fputs(outText, fOut);
		fclose(fOut);
	}
}



