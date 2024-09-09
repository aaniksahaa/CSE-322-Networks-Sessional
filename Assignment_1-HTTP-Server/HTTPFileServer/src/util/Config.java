package util;

import java.io.File;

public class Config {
    // filesystem
    public static final String rootDirName = "root";
    public static final String rootPath = (new File(rootDirName)).getAbsolutePath();

    // server
    public static final String SERVER_NAME = "Java HTTP Server";
    public static final String SERVER_VERSION = "1.0";
    public static final int SERVER_PORT = 5001;

    // status codes
    public static final int HTTP_OK = 200;
    public static final int HTTP_NOT_FOUND = 404;

}
