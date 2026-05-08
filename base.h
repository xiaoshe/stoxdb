#ifndef BASE_H_
#define BASE_H_

#include <string>
#include <vector>

using namespace std;

int scanconf(const vector<string>& conf, const string& key);

// 将字符串str按字符ch切分成多个字符串
void strsplit(const string& str, char ch, vector<string> *out);

// 去掉前后：空格\n\t
void trim(string& str);
void tolower(string& str);

void print(const vector<string> &v);

// 提取字符串开头的数字
int digit(const char *p);

bool isopt(char p);
bool isdigit(char p);
bool isalpha(char p);

int setint1(char *buff, int8_t v);
int setint2(char *buff, int16_t v);
int setint4(char *buff, int v);
int setint8(char *buff, int64_t v);
int setfloat(char *buff, float v);
int setdouble(char *buff, double v);
int setchar1(char *buff, const string& input);
int setchar2(char *buff, const string& input);

int getint1(const char *p, int8_t *out);
int getint2(const char *p, int16_t *out);
int getint4(const char *p, int32_t *out);
int getint8(const char *p, int64_t *out);
int getchar1(const char *p, string *out);
int getchar2(const char *p, string *out);

bool contain(const string& str, const string& sub);
bool startwith(const string& str, const string& sub);
bool endwith(const string& str, const string& sub);

#endif //BASE_H_
