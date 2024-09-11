package util;
import FileSystem.FileSystemNode;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;

public class HtmlGenerator {
    public static String generateDirectoryListingHtml(FileSystemNode node) throws IOException {

        StringBuilder htmlContent = new StringBuilder();

        // Append the HTML header
        htmlContent.append("<!DOCTYPE HTML>\n");
        htmlContent.append("<html>\n");
        htmlContent.append(" <head>\n");
        htmlContent.append("  <title>Index of ").append(node.pathFromRoot).append("</title>\n");
        htmlContent.append(" </head>\n");
        htmlContent.append(" <body>\n");
        htmlContent.append(" <h1>Index of ").append(node.pathFromRoot).append("</h1>\n");
        htmlContent.append(" <ul>\n");

        for(FileSystemNode child: node.getChildren()){
            String hrefTarget = "_self";
            if(!child.isDirectory){
                hrefTarget = "_blank";
            }
            String tagText = child.name;
            if(child.isDirectory){
                tagText = "<b><i>" + child.name + "</i></b>";
            }
            htmlContent.append("  <li><a target=\"" + hrefTarget +"\" href=\"")
                    .append(convertToHtmlPath(child.pathFromRoot))
                    .append("\"> ")
                    .append(tagText)
                    .append("</a></li>\n");
        }

        // Append the closing tags
        htmlContent.append(" </ul>\n");
        htmlContent.append(" </body>\n");
        htmlContent.append("</html>\n");

        // Write the HTML content to the file
        try (BufferedWriter writer = new BufferedWriter(new FileWriter("my.html"))) {
            writer.write(htmlContent.toString());
        }

        return htmlContent.toString();
    }

    public static void main(String[] args) {
        // Example usage
        ArrayList<String> links = new ArrayList<>();
        links.add("ACL.pptx");
        links.add("Basic-Config.pkt");
        links.add("Basic-Config.pptx");
        links.add("NAT.pptx");
        links.add("VLAN.pkt");
        links.add("VLANs.pptx");

        String directory = "/teach/CSE322";

        try {
            FileSystemNode node = new FileSystemNode("/");
            System.out.println(generateDirectoryListingHtml(node));
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    private static String convertToHtmlPath(String path) {
        return path.replace("\\", "/");
    }
}

