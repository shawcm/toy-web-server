#include "http.h"


// return false on error parsing
bool getMethod(const char *&buf, const char *end, HttpMethod &method) {
    debug("getMethod");
    string m;

    while (buf < end) {
        if (*buf == ' ')
            break;
        
        m.push_back(*buf++);
    }
    while (*buf == ' ') buf++;

    if (m == "GET") method = GET;
    else if (m == "HEAD") method = HEAD;
    else if (m == "POST") method = POST;
    else if (m == "PUT") method = PUT;
    else if (m == "DELETE") method = DELETE;
    else if (m == "TRACE") method = TRACE;
    else if (m == "OPTIONS") method = OPTIONS;
    else if (m == "CONNECT") method = CONNECT;
    else if (m == "PATCH") method = PATCH;
    else return false;

    return true;
}

bool getLocation(const char *&buf, const char *end, string &location) {
    debug("getLocation");
    string m;

    while (buf < end) {
        if (*buf == ' ')
            break;
        
        m.push_back(*buf++);
    }
    while (*buf == ' ') buf++;
    location = move(m);
    return true;
}

bool getVersion(const char *&buf, const char *end, string &version) {
    debug("getVersion");
    string m;

    while (buf < end) {
        if (*buf == ' ' || *buf == '\r')
            break;
        
        m.push_back(*buf++);
    }
    while (*buf == ' ' || *buf == '\r' || *buf == '\n') buf++;
    version = move(m);
    return true;
}


inline void updateHeader(string &key, string &value, HttpHeader &header) {
    debug("updateHeader");
    if (key == "A-IM") header.A_IM = move(value);
    else if (key == "Accept") header.Accept = move(value);
    else if (key == "Accept-Charset") header.Accept_Charset = move(value);
    else if (key == "Accept-Datetime") header.Accept_Datetime = move(value);
    else if (key == "Accept-Encoding") header.Accept_Encoding = move(value);
    else if (key == "Accept-Language") header.Accept_Language = move(value);
    else if (key == "Access-Control-Request-Method") header.Access_Control_Request_Method = move(value);
    else if (key == "Access-Control-Request-Headers") header.Access_Control_Request_Headers = move(value);
    else if (key == "Authorization") header.Authorization = move(value);
    else if (key == "Cache-Control") header.Cache_Control = move(value);
    else if (key == "Connection") header.Connection = move(value);
    else if (key == "Content-Encoding") header.Content_Encoding = move(value);
    else if (key == "Content-Length") header.Content_Length = move(value);
    else if (key == "Content-MD5") header.Content_MD5 = move(value);
    else if (key == "Content-Type") header.Content_Type = move(value);
    else if (key == "Cookie") header.Cookie = move(value);
    else if (key == "Date") header.Date = move(value);
    else if (key == "Expect") header.Expect = move(value);
    else if (key == "Forwarded") header.Forwarded = move(value);
    else if (key == "From") header.From = move(value);
    else if (key == "Host") header.Host = move(value);
    else if (key == "HTTP2-Settings") header.HTTP2_Settings = move(value);
    else if (key == "If-Match") header.If_Match = move(value);
    else if (key == "If-Modified-Since") header.If_Modified_Since = move(value);
    else if (key == "If-None-Match") header.If_None_Match = move(value);
    else if (key == "If-Range") header.If_Range = move(value);
    else if (key == "If-Unmodified-Since") header.If_Unmodified_Since = move(value);
    else if (key == "Max-Forwards") header.Max_Forwards = move(value);
    else if (key == "Origin") header.Origin = move(value);
    else if (key == "Pragma") header.Pragma = move(value);
    else if (key == "Prefer") header.Prefer = move(value);
    else if (key == "Proxy-Authorization") header.Proxy_Authorization = move(value);
    else if (key == "Range") header.Range = move(value);
    else if (key == "Referer") header.Referer = move(value);
    else if (key == "TE") header.TE = move(value);
    else if (key == "Trailer") header.Trailer = move(value);
    else if (key == "Transfer-Encoding") header.Transfer_Encoding = move(value);
    else if (key == "User-Agent") header.User_Agent = move(value);
    else if (key == "Upgrade") header.Upgrade = move(value);
    else if (key == "Via") header.Via = move(value);
    else if (key == "Warning") header.Warning = move(value);
    else debug("header %s unknown", key.c_str());
}

bool getHeader(const char *&buf, const char *end, HttpHeader &header) {
    debug("getHeader");
    while (buf < end) {
        string key, value;
        while (buf < end) {
            if (*buf == ' ' || *buf == ':')
                break;
            key.push_back(*buf++);
        }
        while (buf < end && (*buf == ' ' || *buf == ':'))
            buf++;
        while (buf < end) {
            if (*buf == '\r')
                break;
            value.push_back(*buf++);
        }
        debug("key = %s, val = %s\n", key.c_str(), value.c_str());
        updateHeader(key, value, header);

        int cnt = 0;
        while (*buf == '\r' || *buf == '\n') buf++, cnt++;
        if (cnt > 2) break;
    }

    return true;
}

bool getContent(const char *&buf, const char *end, const char *&content, ssize_t &len) {
    debug("get content");
    if (buf > end) return false;

    content = buf;
    len = end - buf;
    return true;
}

HttpRequest::HttpRequest() {}

HttpRequest::HttpRequest(const char *buf, ssize_t len) {
    construct(buf, len);
}


void HttpRequest::construct(const char *buf, ssize_t len) {
    const char *end = buf + len;
    debug("buf = %s\n", buf);
    getMethod(buf, end, method);
    getLocation(buf, end, location);
    getVersion(buf, end, version);
    getHeader(buf, end, header);
    getContent(buf, end, content, content_len_);

    debug("Location(%s), Version(%s)\n", location.c_str(), version.c_str());
}

HttpHeader::HttpHeader(char *buf, ssize_t len) {

}

HttpHeader::HttpHeader() {}

inline void put_header_line(char *&buf, const char *rep, const string &val) {
    debug("put_header_line");
    if (!val.empty()) {
        strncpy(buf, rep, strlen(rep));
        buf += strlen(rep);
        *buf++ = ':';
        *buf++ = ' ';
        strncpy(buf, val.c_str(), val.size());
        buf += val.size();
        *buf++ = '\r';
        *buf++ = '\n';
    }
}

void HttpHeader::dump(char *&buf, ssize_t len) {
    debug("HttpHeader::dump");
    put_header_line(buf, "A-IM", A_IM);
    put_header_line(buf, "Accept", Accept);
    put_header_line(buf, "Accept-Charset", Accept_Charset);
    put_header_line(buf, "Accept-Datetime", Accept_Datetime);
    put_header_line(buf, "Accept-Encoding", Accept_Encoding);
    put_header_line(buf, "Accept-Language", Accept_Language);
    put_header_line(buf, "Access-Control-Request-Method", Access_Control_Request_Method);
    put_header_line(buf, "Access-Control-Request-Headers", Access_Control_Request_Headers);
    put_header_line(buf, "Authorization", Authorization);
    put_header_line(buf, "Cache-Control", Cache_Control);
    put_header_line(buf, "Connection", Connection);
    put_header_line(buf, "Content-Encoding", Content_Encoding);
    put_header_line(buf, "Content-Length", Content_Length);
    put_header_line(buf, "Content-MD5", Content_MD5);
    put_header_line(buf, "Content-Type", Content_Type);
    put_header_line(buf, "Cookie", Cookie);
    put_header_line(buf, "Date", Date);
    put_header_line(buf, "Expect", Expect);
    put_header_line(buf, "Forwarded", Forwarded);
    put_header_line(buf, "From", From);
    put_header_line(buf, "Host", Host);
    put_header_line(buf, "HTTP2-Settings", HTTP2_Settings);
    put_header_line(buf, "If-Match", If_Match);
    put_header_line(buf, "If-Modified-Since", If_Modified_Since);
    put_header_line(buf, "If-None-Match", If_None_Match);
    put_header_line(buf, "If-Range", If_Range);
    put_header_line(buf, "If-Unmodified-Since", If_Unmodified_Since);
    put_header_line(buf, "Max-Forwards", Max_Forwards);
    put_header_line(buf, "Origin", Origin);
    put_header_line(buf, "Pragma", Pragma);
    put_header_line(buf, "Prefer", Prefer);
    put_header_line(buf, "Proxy-Authorization", Proxy_Authorization);
    put_header_line(buf, "Range", Range);
    put_header_line(buf, "Referer", Referer);
    put_header_line(buf, "TE", TE);
    put_header_line(buf, "Trailer", Trailer);
    put_header_line(buf, "Transfer-Encoding", Transfer_Encoding);
    put_header_line(buf, "User-Agent", User_Agent);
    put_header_line(buf, "Upgrade", Upgrade);
    put_header_line(buf, "Via", Via);
    put_header_line(buf, "Warning", Warning);
}


HttpRequestProgressBuilder::HttpRequestProgressBuilder(char *buf) :buf(buf) {
    skip = buf;
    
    content_len = 0;
    current_len = 0;

    current_content_len = 0;
    sequence_r_r = 0;
    prev_is_r_n = false;
    state = READ_INIT;
}

void HttpRequestProgressBuilder::reset() {
    memmove(buf, skip, current_content_len);
    current_len = current_content_len;
    content_len = 0;
    skip = buf;
    sequence_r_r = 0;
    state = READ_INIT;
}

// return true when finish
// after each len = read(), call progress(len), and
// when progress return true, use HttpRequest() to get the request
int HttpRequestProgressBuilder::progress(ssize_t len) {
    debug("builder progress len = %u, (current_len=%d, state=%d) %s", len, current_len, state, buf);
    current_len += len;
    const char *end = buf + current_len;

    if (state == READ_INIT && len >= 3) {

        if (strncmp(buf, "GET", 3) == 0) method = GET, state = READ_LENGTH;
        else if (strncmp(buf, "POST", 3) == 0) method = POST, state = READ_BEGIN;
        else return -1; // unknown method
        // other methods
    }

    if (state == READ_BEGIN) {
        const char *line_beg = skip;
        while (line_beg < end && (*line_beg == '\n' || *line_beg == '\r')) line_beg++;
        const char *cursor = line_beg;

        // 每次读一行 \r\n
        while (cursor < end) {
            while (cursor < end && *cursor != '\n') cursor++;
            if (cursor >= end) return 0;
            // 找到了一行，从 line_beg 到 cursor
            // 检查 beg~line_beg 之间有没有 content-length 
            for (const char *p = line_beg; p < cursor; p++) {
                if (
                       *(p+0) == 'C'
                    && *(p+1) == 'o'
                    && *(p+2) == 'n'
                    && *(p+3) == 't'
                    && *(p+4) == 'e'
                    && *(p+5) == 'n'
                    && *(p+6) == 't'
                    && *(p+7) == '-'
                    && *(p+8) == 'L'
                    && *(p+9) == 'e'
                    && *(p+10) == 'n'
                    && *(p+11) == 'g'
                    && *(p+12) == 't'
                    && *(p+13) == 'h') {
                    // found content
                    p += 14;
                    while (*p == ' ' || *p == ':') p++;

                    const char *pend = p;
                    ssize_t len = 0;
                    while (*pend >= '0' && *pend <= '9') 
                        len = len * 10 + (*pend)-'0';

                    state = READ_LENGTH;
                    skip = pend;
                    while (skip < end && *skip != '\r') skip++;
                    sequence_r_r = 0;
                    while (skip < end && (*skip == '\r' || *skip == '\n')) prev_is_r_n = true, skip++, sequence_r_r++;
                    break;
                }
            }

            if (sequence_r_r == 4) {
                state = READ_CONTENT;
                current_content_len = end - skip;
            }

            if (state == READ_LENGTH || state == READ_CONTENT) return 0;

            while (*cursor == '\r' || *cursor == '\n') cursor++;
            line_beg = cursor;
            skip = line_beg;
        }
    } else if (state == READ_LENGTH) {
        for (const char *p = skip; p < end; p++) {
            if (*p != '\r' && *p != '\n') {
                prev_is_r_n = false;
                sequence_r_r = 0;
            } else {
                prev_is_r_n = true;
                sequence_r_r++;
            }

            if (sequence_r_r == 4) {
                state = READ_CONTENT;
                current_content_len = end - p - 1;
                skip = p+1;
                if (method == GET) return 1;
                break;
            }
        }
    } else if (state == READ_CONTENT) {
        current_content_len += len;
        if (current_content_len == content_len) return 1;
    }
    debug("finish builder progress");
    return 0;
}

// TODO buffer size check
ssize_t HttpResponse::dump(char *buf, ssize_t _len) {
    debug("httpResponse dump");
    ssize_t len = 0;
    char *oldbuf = buf;

    char *end = buf + _len;

    len = version.size();
    strncpy(buf, version.c_str(), len);
    buf += len;

    *buf++ = ' ';
    const char *state_str = httpStatusToString(status);
    len = strlen(state_str);
    strncpy(buf, state_str, len);
    buf += len;
    *buf++ = '\r'; *buf++ = '\n';

    header.dump(buf, end - buf);

    *buf++ = '\r'; *buf++ = '\n'; // *buf++ = '\r'; *buf++ = '\n';
    if (content && content_len_) strncpy(buf, content, content_len_);

    return buf - oldbuf + content_len_;
}


const char* httpStatusToString (HttpStatus s) {
    debug("httpStatusToString");
    switch (s)
    {
    case OK:
        return "200 OK";
    case MovedPermanently:
        return "301 Moved Permanently"; 
    case BadRequest:
        return "400 Bad Request";
    case NotFound:
        return "404 Not Found";
    case RequestTimeout:
        return "408 Request Timeout";
    default:
        break;
    }
    return NULL;
}

////
// #define accept_main_test
#ifdef accept_main_test

void _test_builder() {
    const char *req = "\x47\x45\x54\x20\x2f\x20\x48\x54\x54\x50\x2f\x31\x2e\x31\x0d\x0a\x48\x6f\x73\x74\x3a\x20\x6c\x6f\x63\x61\x6c\x68\x6f\x73\x74\x3a\x32\x33\x37\x37\x0d\x0a\x55\x73\x65\x72\x2d\x41\x67\x65\x6e\x74\x3a\x20\x4d\x6f\x7a\x69\x6c\x6c\x61\x2f\x35\x2e\x30\x20\x28\x58\x31\x31\x3b\x20\x55\x62\x75\x6e\x74\x75\x3b\x20\x4c\x69\x6e\x75\x78\x20\x78\x38\x36\x5f\x36\x34\x3b\x20\x72\x76\x3a\x38\x38\x2e\x30\x29\x20\x47\x65\x63\x6b\x6f\x2f\x32\x30\x31\x30\x30\x31\x30\x31\x20\x46\x69\x72\x65\x66\x6f\x78\x2f\x38\x38\x2e\x30\x0d\x0a\x41\x63\x63\x65\x70\x74\x3a\x20\x74\x65\x78\x74\x2f\x68\x74\x6d\x6c\x2c\x61\x70\x70\x6c\x69\x63\x61\x74\x69\x6f\x6e\x2f\x78\x68\x74\x6d\x6c\x2b\x78\x6d\x6c\x2c\x61\x70\x70\x6c\x69\x63\x61\x74\x69\x6f\x6e\x2f\x78\x6d\x6c\x3b\x71\x3d\x30\x2e\x39\x2c\x69\x6d\x61\x67\x65\x2f\x77\x65\x62\x70\x2c\x2a\x2f\x2a\x3b\x71\x3d\x30\x2e\x38\x0d\x0a\x41\x63\x63\x65\x70\x74\x2d\x4c\x61\x6e\x67\x75\x61\x67\x65\x3a\x20\x7a\x68\x2c\x65\x6e\x2d\x55\x53\x3b\x71\x3d\x30\x2e\x37\x2c\x65\x6e\x3b\x71\x3d\x30\x2e\x33\x0d\x0a\x41\x63\x63\x65\x70\x74\x2d\x45\x6e\x63\x6f\x64\x69\x6e\x67\x3a\x20\x67\x7a\x69\x70\x2c\x20\x64\x65\x66\x6c\x61\x74\x65\x0d\x0a\x43\x6f\x6e\x6e\x65\x63\x74\x69\x6f\x6e\x3a\x20\x6b\x65\x65\x70\x2d\x61\x6c\x69\x76\x65\x0d\x0a\x55\x70\x67\x72\x61\x64\x65\x2d\x49\x6e\x73\x65\x63\x75\x72\x65\x2d\x52\x65\x71\x75\x65\x73\x74\x73\x3a\x20\x31\x0d\x0a\x43\x61\x63\x68\x65\x2d\x43\x6f\x6e\x74\x72\x6f\x6c\x3a\x20\x6d\x61\x78\x2d\x61\x67\x65\x3d\x30\x0d\x0a\x0d\x0a";
    printf("%s", req);
    HttpRequestProgressBuilder builder(req);
    int cnt = strlen(req);

    assert (builder.progress(cnt) == 1);
    HttpRequest hr(req, strlen(req));
}

void _test_response() {
    HttpResponse response;
    response.status = OK;
    response.version = "HTTP/1.1";
    response.header.From = "somehost.com";
    const char *content_buf = "the content";
    response.content = content_buf;
    response.content_len_ = strlen(content_buf);

    char *buf = new char[BUFSIZ];
    response.dump(buf, BUFSIZ);
    puts(buf);
}

int main() {
    // const char *req = "GET /questions/25896916/parse-http-headers-in-c HTTP/2\r\nHost: stackoverflow.com\r\nUser-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:88.0) Gecko/20100101 Firefox/88.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nAccept-Language: zh,en-US;q=0.7,en;q=0.3\r\nAccept-Encoding: gzip, deflate, br\r\nReferer: https://www.google.com/\r\nConnection: keep-alive\r\nCookie: prov=ccabd2a5-e95f-e1e6-8d35-d6cef56a7a00; OptanonConsent=isIABGlobal=false&datestamp=Tue+May+04+2021+13%3A12%3A41+GMT%2B0800+(China+Standard+Time)&version=6.10.0&hosts=&landingPath=NotLandingPage&groups=C0003%3A1%2CC0004%3A1%2CC0002%3A1%2CC0001%3A1; OptanonAlertBoxClosed=2021-05-04T05:12:41.250Z; hero-dismissed=1620105210129!stk_a\r\nUpgrade-Insecure-Requests: 1\r\nCache-Control: max-age=0\r\nTE: Trailers\r\n\r\nhello,man";
    // HttpRequestProgressBuilder builder(req);
    // int cnt = 10;
    // while (builder.progress(10) == 0) {
    //     //snprintf((char*)req, cnt, "%s\n", req);
    //     printf("%d, status = %d\n", cnt, builder.state);
    //     cnt += 10;
    //     puts("");
    // }
    // HttpRequest hr(req, strlen(req));
    _test_builder();
}
#endif
