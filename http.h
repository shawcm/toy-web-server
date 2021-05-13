#ifndef HTTP_H
#define HTTP_H

#include "util.h"
struct HttpHeader;
struct HttpRequest;


enum HttpMethod {
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    TRACE,
    OPTIONS,
    CONNECT,
    PATCH
};


enum HttpStatus {
    OK = 200,
    MovedPermanently = 301,
    BadRequest = 400,
    NotFound = 404,
    RequestTimeout = 408
    // and lots of others, to be added
};

const char* httpStatusToString (HttpStatus s);

// the header fields of a HTTP request
struct HttpHeader {
    // constructor: a buffer location, buffer size
    HttpHeader();
    HttpHeader(char *buf, ssize_t len); 
    
    // write header to a buffer
    void dump(char *&buf, ssize_t len);

    string A_IM;
    string Accept;
    string Accept_Charset;
    string Accept_Datetime;
    string Accept_Encoding;
    string Accept_Language;
    string Access_Control_Request_Method;
    string Access_Control_Request_Headers;
    string Authorization;
    string Cache_Control;
    string Connection;
    string Content_Encoding;
    string Content_Length;
    string Content_MD5;
    string Content_Type;
    string Cookie;
    string Date;
    string Expect;
    string Forwarded;
    string From;
    string Host;
    string HTTP2_Settings;
    string If_Match;
    string If_Modified_Since;
    string If_None_Match;
    string If_Range;
    string If_Unmodified_Since;
    string Max_Forwards;
    string Origin;
    string Pragma;
    string Prefer;
    string Proxy_Authorization;
    string Range;
    string Referer;
    string TE;
    string Trailer;
    string Transfer_Encoding;
    string User_Agent;
    string Upgrade;
    string Via;
    string Warning;
};


struct HttpRequest {
    typedef std::shared_ptr<HttpRequest> Ptr;
    HttpMethod method;      // GET
    string location;        // /index.html
    string version;         // HTTP/1.1
    HttpHeader header;      // heads..
    const char *content;          // content buffer
    ssize_t content_len_;

    // construct the struct from buffer
    HttpRequest();
    HttpRequest(const char *buf, ssize_t len);
    void construct(const char *buf, ssize_t len);
};

enum HttpRequestProgressBuildState {
    READ_INIT,
    READ_BEGIN,
    READ_LENGTH,
    READ_CONTENT,
    READ_FINISH
};


struct HttpRequestProgressBuilder {
    typedef std::shared_ptr<HttpRequestProgressBuilder> Ptr;

    HttpRequestProgressBuildState state;
    char *buf;
    const char *skip;
    HttpMethod method;
    ssize_t current_len;
    ssize_t content_len;
    ssize_t current_content_len;
    ssize_t sequence_r_r;
    bool prev_is_r_n;
    HttpRequestProgressBuilder(char *buf);
    int progress(ssize_t len);
    void reset();
};


struct HttpResponse {
    typedef std::shared_ptr<HttpResponse> Ptr;
    string version;
    HttpStatus status;
    HttpHeader header;
    char *content;
    ssize_t content_len_;

    //HttpResponse(char *buf, ssize_t len); ///?no use
    // write response to a buffer
    ssize_t dump(char *buf, ssize_t len);
};



#endif