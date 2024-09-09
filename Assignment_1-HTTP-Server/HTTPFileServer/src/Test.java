import FileSystem.FileSystemNode;
import util.Config;

public class Test {
    public static void main(String[] args) {
        String path = "/"; // Replace with your root path
        String pathFromRoot = path;
        FileSystemNode node = new FileSystemNode(pathFromRoot);
        System.out.println(node);
        for(FileSystemNode c: node.getChildren()){
            System.out.println(c);
        }
    }
}
