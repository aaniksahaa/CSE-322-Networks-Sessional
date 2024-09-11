package util;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;

public class Helper {
    public static String getContentType(File file) throws IOException {
        String contentType = Files.probeContentType(file.toPath());
        if (contentType == null) {
            contentType = "application/octet-stream";
        }
        return contentType;
    }

}
