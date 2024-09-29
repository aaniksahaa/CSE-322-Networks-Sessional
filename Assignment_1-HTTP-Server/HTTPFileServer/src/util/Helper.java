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

    public static void initializeUploadDir(){
        File uploadDirectory = new File(Config.uploadDir);
        makeDir(uploadDirectory);
    }

    public static void makeDir(File directory){
        System.out.println("\nChecking Directory: "+directory.getName());
        if (!directory.exists()) {
            if (directory.mkdirs()) {
                System.out.println("Directory created successfully.");
            } else {
                System.out.println("Failed to create the directory.");
            }
        } else {
            System.out.println("Directory already exists.");
        }
        System.out.println("");
    }

}
