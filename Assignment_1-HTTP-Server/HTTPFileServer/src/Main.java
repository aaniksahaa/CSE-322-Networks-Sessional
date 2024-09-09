import java.io.File;

public class Main {

    public static void main(String[] args) {
        String rootDirName = "root";
        // Example usage:
        String path = "/"; // Replace with your root path
        String pathFromRoot = rootDirName + path;

        File rootDir = new File(rootDirName);
        File file = new File(pathFromRoot);

        if (file.exists()) {
            // Start listing files and directories relative to the root
            listFilesAndDirs(file, rootDir.getAbsolutePath());
        } else {
            System.out.println("Root directory does not exist.");
        }
    }

    public static void listFilesAndDirs(File file, String rootPath) {
        // Check if it is a directory
        if (file.isDirectory()) {
            System.out.println("Directory: " + getRelativePath(file, rootPath));

            // Get all files and directories within the directory
            File[] contents = file.listFiles();

            if (contents != null) {
                for (File f : contents) {
                    if (f.isDirectory()) {
                        // Recursively traverse directories
                        listFilesAndDirs(f, rootPath);
                    } else {
                        // If it's a file, print its name
                        System.out.println("File: " + getRelativePath(f, rootPath));
                    }
                }
            }
        } else {
            // If it's a file, just print its name
            System.out.println("File: " + getRelativePath(file, rootPath));
        }
    }

    // Helper method to get the relative path with respect to the root directory
    public static String getRelativePath(File file, String rootPath) {
        String absolutePath = file.getAbsolutePath();
        return absolutePath.substring(rootPath.length());
    }
}
