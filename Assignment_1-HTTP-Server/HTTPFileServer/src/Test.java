import FileSystem.Node;
import util.Config;

public class Test {
    public static void main(String[] args) {
        String path = "/dir1"; // Replace with your root path
        String pathFromRoot = Config.rootDirName + path;
        Node node = new Node(pathFromRoot);
        System.out.println(node.isValid);
        for(Node c: node.getChildren()){
            System.out.println(c);
        }
    }
}
