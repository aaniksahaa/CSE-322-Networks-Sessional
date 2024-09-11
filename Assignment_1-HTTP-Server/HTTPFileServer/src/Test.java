import FileSystem.FileSystemNode;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.file.Files;

public class Test {
    public static void main(String[] args) throws IOException{
        String filePath = "/v.mp4";
        FileSystemNode node = new FileSystemNode(filePath);
        System.out.println(node.name);
    }
}
