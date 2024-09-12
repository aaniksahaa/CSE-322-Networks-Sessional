package util;

import java.io.IOException;
import java.nio.file.Files;
import java.util.Base64;

public class HtmlGenerator {
    public static String generateDirectoryListingHtml(FileSystemNode node) throws IOException {
        StringBuilder htmlBuilder = new StringBuilder();

        String title = "Index of " + node.pathFromRoot;

        writePrimer(htmlBuilder, title);

        htmlBuilder.append(" <ul>\n");

        for(FileSystemNode child: node.getChildren()){
            String hrefTarget = "_self";
            if((!child.isDirectory) && child.isTextOrImage()){
                hrefTarget = "_blank";
            }
            String tagText = child.name;
            if(child.isDirectory){
                tagText = "<b><i>" + child.name + "</i></b>";
            }
            htmlBuilder.append("  <li><a target=\"" + hrefTarget +"\" href=\"")
                    .append(convertToHtmlPath(child.pathFromRoot))
                    .append("\"> ")
                    .append(tagText)
                    .append("</a></li>\n");
        }

        htmlBuilder.append(" </ul>\n");

        writeClosing(htmlBuilder);

        return htmlBuilder.toString();
    }
    public static String generateTextHtml(FileSystemNode node) throws IOException {
        StringBuilder htmlBuilder = new StringBuilder();

        String title = "Content of " + node.pathFromRoot;

        writePrimer(htmlBuilder, title);

        String textContent = new String(Files.readAllBytes(node.file.toPath()));
        htmlBuilder.append(" <pre>").append(textContent).append("</pre>\n");

        writeClosing(htmlBuilder);

        return htmlBuilder.toString();
    }
    public static String generateImageHtml(FileSystemNode node) throws IOException{
        StringBuilder htmlBuilder = new StringBuilder();

        String title = "Content of " + node.pathFromRoot;

        writePrimer(htmlBuilder, title);

        int maxWidth = 1280;
        int maxHeight = 720;

        byte[] imageBytes = Files.readAllBytes(node.file.toPath());
        String base64Image = Base64.getEncoder().encodeToString(imageBytes);
        htmlBuilder.append("<img src='data:")
                .append(node.contentType)
                .append(";base64,")
                .append(base64Image)
                .append("' style='max-width:").append(maxWidth)
                .append("px; max-height:").append(maxHeight)
                .append("px; display: block; margin: 0 auto;")
                .append("' />");

        writeClosing(htmlBuilder);

        return htmlBuilder.toString();
    }
    private static String convertToHtmlPath(String path) {
        return path.replace("\\", "/");
    }

    public static void writePrimer(StringBuilder htmlBuilder, String title) {
        htmlBuilder.append("<!DOCTYPE HTML>\n");
        htmlBuilder.append("<html>\n");
        htmlBuilder.append(" <head>\n");
        htmlBuilder.append("  <title>").append(title).append("</title>\n");
        htmlBuilder.append(" </head>\n");
        htmlBuilder.append(" <body>\n");
        htmlBuilder.append(" <h1>").append(title).append("</h1>\n");
    }
    public static void writeNewline(StringBuilder htmlBuilder) {
        htmlBuilder.append(" <br/>");
    }
    public static void writeClosing(StringBuilder htmlBuilder) {
        htmlBuilder.append(" </body>\n");
        htmlBuilder.append("</html>\n");
    }
    public static void main(String[] args) {
        try {
            FileSystemNode node = new FileSystemNode("/");
            System.out.println(generateDirectoryListingHtml(node));

            node = new FileSystemNode("/file1.txt");
            System.out.println(generateTextHtml(node));
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

}

