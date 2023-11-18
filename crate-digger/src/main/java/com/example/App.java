package com.example;
import org.deepsymmetry.cratedigger.Database;
import java.io.File;
import java.io.IOException;
/**
 * Hello world!
 *
 */
public class App 
{
    public static void main( String[] args )
    {
        File pdb = new File("./src/main/java/com/example/export.pdb");
        System.out.println(pdb);
        try {
            Database database = new Database(pdb);
            System.out.println("success");
        
        } catch (IOException e) {
            //
            System.out.println(e);
        }
    }
}
