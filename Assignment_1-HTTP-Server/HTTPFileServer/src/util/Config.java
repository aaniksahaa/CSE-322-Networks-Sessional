package util;

import java.io.File;

public class Config {
    public static final String HTTP_VERSION = "HTTP/1.1";
    // filesystem
    public static final String rootDirName = "root";
    public static final String rootPath = (new File(rootDirName)).getAbsolutePath();

    // server
    public static final String SERVER_NAME = "Java HTTP Server: 1.0";
    public static final int SERVER_PORT = 5001;

    // status codes
    public static final Status OK = new Status(200, "OK");
    public static final Status BAD_REQUEST = new Status(400, "Bad Request");
    public static final Status NOT_FOUND = new Status(404, "Not Found");
    public static final Status INTERNAL_SERVER_ERROR = new Status(500, "Internal Server Error");

    public static final int CHUNK_SIZE = 1024;

}
