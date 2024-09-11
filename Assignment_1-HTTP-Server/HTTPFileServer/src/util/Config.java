package util;

import java.io.File;

public class Config {
    public static final String HTTP_VERSION = "HTTP/1.1";
    // filesystem
    public static final String rootDirName = "root";
    public static final String rootPath = (new File(rootDirName)).getAbsolutePath();

    // server
    public static final String SERVER_NAME = "Java HTTP Server: 1.0";
    public static final String SERVER_HOST = "localhost";
    public static final int SERVER_PORT = 5001;

    // errors
    public static final String UPLOAD_FILE_FORMAT_ERROR = "Invalid filename or format. Only text and image files are accepted.";

    // status codes
    public static final HttpStatus OK = new HttpStatus(200, "OK");
    public static final HttpStatus BAD_REQUEST = new HttpStatus(400, "Bad Request");
    public static final HttpStatus NOT_FOUND = new HttpStatus(404, "Not Found");
    public static final HttpStatus INTERNAL_SERVER_ERROR = new HttpStatus(500, "Internal Server Error");

    public static final int CHUNK_SIZE = 8192;

    public static final int artificalSendingDelayMs = 5;

}
