#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <regex.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 去掉新闻标题首尾的空白字符
void trim(char* str)
{
	char* begin = str;
	char* end = NULL;

	while (*begin && isspace((unsigned char)*begin)) {
		begin++;
	}

	if (begin != str) {
		memmove(str, begin, strlen(begin) + 1);
	}

	if ('\0' == str[0]) {
		return;
	}

	end = str + strlen(str) - 1;
	while (end >= str && isspace((unsigned char)*end)) {
		*end = '\0';
		end--;
	}
}

int main(int argc, char** argv)
{
	const char* filename = "url.txt";
	if (argc > 1) {
		filename = argv[1];
	}

	// 1、打开文件
	int fd = open(filename, O_RDONLY);
	if (-1 == fd) {
		perror("open");
		return 1;
	}

	// 2、获取文件大小
	int fsize = lseek(fd, 0, SEEK_END);
	if (fsize <= 0) {
		perror("lseek");
		close(fd);
		return 1;
	}

	// 3、将文件映射到内存
	char* mmap_ptr = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (MAP_FAILED == mmap_ptr) {
		perror("mmap");
		return 1;
	}

	// regexec要求数据以'\0'结尾，复制一份并补上字符串结束符
	char* data = (char*)malloc(fsize + 1);
	if (NULL == data) {
		perror("malloc");
		munmap(mmap_ptr, fsize);
		return 1;
	}
	memcpy(data, mmap_ptr, fsize);
	data[fsize] = '\0';
	munmap(mmap_ptr, fsize);

	// 4、生成正则
	char* regstr =
		"<a[^>]*href[[:space:]]*=[[:space:]]*[\"']\\([^\"'][^\"']*\\)[\"'][^>]*>[[:space:]]*\\([^<][^<]*\\)[[:space:]]*</a>";
	int regnum = 3;
	regmatch_t match[3];
	regex_t reg;
	int ret = regcomp(&reg, regstr, 0);
	if (0 != ret) {
		char error_buff[1024] = { 0 };
		regerror(ret, &reg, error_buff, sizeof(error_buff));
		printf("regcomp error: %s\n", error_buff);
		free(data);
		return 1;
	}

	// 5、循环匹配并提取新闻地址和新闻标题
	char* current = data;
	int count = 0;
	while (0 == regexec(&reg, current, regnum, match, 0)) {
		int url_len = match[1].rm_eo - match[1].rm_so;
		int title_len = match[2].rm_eo - match[2].rm_so;

		char* url = (char*)malloc(url_len + 1);
		char* title = (char*)malloc(title_len + 1);
		if (NULL == url || NULL == title) {
			perror("malloc");
			free(url);
			free(title);
			break;
		}

		memcpy(url, current + match[1].rm_so, url_len);
		url[url_len] = '\0';
		memcpy(title, current + match[2].rm_so, title_len);
		title[title_len] = '\0';
		trim(title);

		printf("新闻地址：%s\n", url);
		printf("新闻标题：%s\n\n", title);
		count++;

		free(url);
		free(title);

		// 偏移到本次完整匹配结果之后，继续匹配下一条
		current += match[0].rm_eo;
	}

	printf("共匹配到 %d 条有效新闻链接。\n", count);

	// 6、释放资源
	regfree(&reg);
	free(data);
	return 0;
}
