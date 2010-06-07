#include <string.h>
#include <stdio.h>
#include <iostream>
#include <functional>
#include <algorithm>
#include <set>

#define MAX_READ 1000
#define MAX_STRING 100

typedef enum protocols_e {
    HTTP,
    HTTPS,
    FTP,
    TELNET,
    PROTOCOL_MAX
} protocols;

typedef enum linkstart_e {
    HREF_1,
    HREF_2,
    HREF_3,
    HREF_4,
    HREF_MAX
} linkstart;

typedef struct url_t_ {
    std::string type;
    std::string host;
    std::string file;
} url_t;

struct lturl {
  bool operator() (const url_t& lhs, const url_t& rhs) const
  {
    return (strcmp(lhs.host.c_str(), rhs.host.c_str()) < 0);
  }
};

url_t
parse_single_URL(std::string site);

void
parse_URLs(char *buffer, int size, std::set<url_t, lturl, std::allocator<url_t> > &urlSet); 

std::string
inputSite(const char *argument);
