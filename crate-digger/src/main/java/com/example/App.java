package com.example;
import org.deepsymmetry.cratedigger.Database;
import org.deepsymmetry.cratedigger.pdb.*;
import java.io.File;
import java.io.IOException;
import java.util.*;


public class App 
{
    public static void main( String[] args )
    {
        File pdb = new File("./export.pdb");
        try {
            Database database = new Database(pdb);
            Map<Long, RekordboxPdb.TrackRow> map = database.trackIndex;
            for (Map.Entry<Long, RekordboxPdb.TrackRow> entry : map.entrySet()) {
                Long key = entry.getKey();
                RekordboxPdb.TrackRow row = entry.getValue();
                System.out.println("key:" + key);
                //Only use database
                System.out.println("filename:" + Database.getText(row.filePath()));
                System.out.println(row.tempo());
            }
            System.out.println("size is " + map.size());
        } catch (IOException e) {
            System.out.println(e);
        }
    }
}
