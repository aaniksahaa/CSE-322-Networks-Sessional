package FileSystem;

import util.Config;
import util.Helper;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.ArrayList;

public class FileSystemNode {
    public String name;
    public Boolean isValid;
    public Boolean isDirectory;
    public String type;
    public String pathFromRoot;

    public File file;
    public String contentType;

    // this path starts after root directory
    // so '/' means the whole root dir, '/dir1' means dir1 under root
    public FileSystemNode(String pathFromRoot) throws IOException {
        this.pathFromRoot = pathFromRoot;
        // here, no stream is actually being opened
        // so no need to close
        // just a File object, storing it in the node object for convenience
        File file = new File(Config.rootDirName + pathFromRoot);
        this.file = file;
        if (file.exists()) {
            this.name = file.getName();
            this.isValid = true;
            if(file.isDirectory()){
                this.isDirectory = true;
                this.type = "dir";
            }
            else {
                this.isDirectory = false;
                this.type = getFileExtension(this.name);
                this.contentType = Helper.getContentType(file);
            }
        } else {
            this.isValid = false;
        }
    }

    // only gives own children, not recursive
    public ArrayList<FileSystemNode> getChildren() throws IOException{
        ArrayList<FileSystemNode>children = new ArrayList<>();
        if(file.exists() && file.isDirectory()){
            File[] contents = file.listFiles();
            if (contents != null) {
                for (File f : contents) {
                    FileSystemNode child = new FileSystemNode(getRelativePath(f, Config.rootPath));
                    children.add(child);
                }
            }
        }
        return children;
    }
    public Boolean isTextOrImage(){
        if (contentType.startsWith("text/") || contentType.startsWith("image/")) {
            return true;
        }
        return false;
    }
    public Boolean isTextOrImageOrVideo(){
        if (contentType.startsWith("text/") || contentType.startsWith("image/") || contentType.startsWith("video/")) {
            return true;
        }
        return false;
    }
    private String getRelativePath(File file, String rootPath) {
        String absolutePath = file.getAbsolutePath();
        return absolutePath.substring(rootPath.length());
    }
    private String getFileExtension(String fileName) {
        int lastDotIndex = fileName.lastIndexOf('.');

        if (lastDotIndex == -1 || lastDotIndex == 0) {
            return "";
        }

        return fileName.substring(lastDotIndex + 1);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("\n")
                .append("name='").append(name).append('\'')
                .append(", isValid=").append(isValid)
                .append(", isDirectory=").append(isDirectory)
                .append(", type='").append(type).append('\'')
                .append(", pathFromRoot='").append(pathFromRoot).append('\'')
                .append('\n');
        return sb.toString();
    }
}
