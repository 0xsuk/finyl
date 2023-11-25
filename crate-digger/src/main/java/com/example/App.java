package com.example;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.deepsymmetry.cratedigger.Database;
import org.deepsymmetry.cratedigger.pdb.*;
import io.kaitai.struct.RandomAccessFileKaitaiStream;
import java.io.File;
import java.io.IOException;
import java.util.*;

class Out {
    private static ObjectMapper objm = new ObjectMapper();
    public String error = null;
    public ArrayList<String> playlists = null;
    
    public Out() {
    }

    public void print() {
        try {
            System.out.println(objm.writeValueAsString(this));
        } catch (JsonProcessingException e) {
        }
    }
}

public class App {
    //section.body() is either
    // org.deepsymmetry.cratedigger.pdb.RekordboxAnlz$PathTag
    // org.deepsymmetry.cratedigger.pdb.RekordboxAnlz$VbrTag
    // org.deepsymmetry.cratedigger.pdb.RekordboxAnlz$BeatGridTag
    // org.deepsymmetry.cratedigger.pdb.RekordboxAnlz$WavePreviewTag
    // org.deepsymmetry.cratedigger.pdb.RekordboxAnlz$WavePreviewTag
    // org.deepsymmetry.cratedigger.pdb.RekordboxAnlz$CueTag
    // org.deepsymmetry.cratedigger.pdb.RekordboxAnlz$CueTag

    
    public static RekordboxAnlz.BeatGridTag findBeatGridTag(RekordboxAnlz anlz) {
        for (RekordboxAnlz.TaggedSection section : anlz.sections()) {
            if (section.body() instanceof RekordboxAnlz.BeatGridTag) {
                return (RekordboxAnlz.BeatGridTag) section.body();
            }
        }

        return null;
    }
    
    public static void printBeatGridBeat(RekordboxAnlz.BeatGridBeat bgbeat) {
        System.out.println("\tbeatNumber:" + bgbeat.beatNumber());
        System.out.println("\ttempo:" + bgbeat.tempo());
        System.out.println("\ttime:" + bgbeat.time());
    }
    
    public static void printBeatGridTag(RekordboxAnlz.BeatGridTag bgtag) {
        System.out.println("lenBeats:" + bgtag.lenBeats());
    }
    
    public static ArrayList<RekordboxAnlz.CueTag> findCueTag(RekordboxAnlz anlz) {
        ArrayList<RekordboxAnlz.CueTag> cuetags = new ArrayList<>();
        for (RekordboxAnlz.TaggedSection section : anlz.sections()) {
            if (section.body() instanceof RekordboxAnlz.CueTag) {
                RekordboxAnlz.CueTag cuetag = (RekordboxAnlz.CueTag) section.body();
                cuetags.add(cuetag);
            }
        }

        return cuetags;
    }
    
    public static void printCueEntry(RekordboxAnlz.CueEntry entry) {
        System.out.println("\thotCue:" + entry.hotCue());
        System.out.println("\ttime:" + entry.time());
        System.out.println("\ttype:" + entry.type());
    }
    
    public static void printCueTag(RekordboxAnlz.CueTag cuetag) {
        System.out.println("lenCues:" + cuetag.lenCues());
        System.out.println("memoryCount:" + cuetag.memoryCount());
        System.out.println("type:" + cuetag.type().id());
        
        if (cuetag.lenCues() > 0) {
            sortCueEntries(cuetag.cues());
            for (RekordboxAnlz.CueEntry entry : cuetag.cues()) {
                printCueEntry(entry);
            }
        }
    }
    
    public static void sortCueEntries(ArrayList<RekordboxAnlz.CueEntry> entries) {
        Collections.sort(entries, (entry1, entry2) -> Long.compare(entry1.time(), entry2.time()));
    }
    
    public static ArrayList<RekordboxAnlz.CueEntry> getSortedCueEntries(ArrayList<RekordboxAnlz.CueTag> cuetags) {
        ArrayList<RekordboxAnlz.CueEntry> entries = new ArrayList<>();

        for (RekordboxAnlz.CueTag cuetag : cuetags) {
            if (cuetag.lenCues() > 0) {
                entries.addAll(cuetag.cues());
            }
        }

        sortCueEntries(entries);
        return entries;
    }
    
    public static void outPlaylists(String filepath) {
        Out out = new Out();
        out.playlists = new ArrayList<>();
        out.playlists.add("filepath:" + filepath);
        out.print();
    }
    
    
    public static void outError(String error) {
        Out out = new Out();
        out.error = error;
        out.print();
    }
    
    public static void main(String[] args) {
        String op = args[0];
        
        switch (op) {
        case "playlists":
            outPlaylists("asdfasdf");
            break;
        default:
            outError("command usage");
        }

        // File pdb = new File("./export.pdb");
        // try {
        //     Database database = new Database(pdb);
        //     Map<Long, RekordboxPdb.TrackRow> map = database.trackIndex;
        //     for (Map.Entry<Long, RekordboxPdb.TrackRow> entry : map.entrySet()) {
        //         Long key = entry.getKey();
        //         RekordboxPdb.TrackRow row = entry.getValue();
        //         String path = "/media/null/22BC-F655" + Database.getText(row.filePath());
        //         String anlzPath = "/media/null/22BC-F655" + Database.getText(row.analyzePath());
        //         RekordboxAnlz anlz = new RekordboxAnlz(new RandomAccessFileKaitaiStream(anlzPath));

        //         // if (!path.contains("Woman Trouble")) {
        //         // continue;
        //         // }
                
        //         if (key != 203) {
        //             continue;
        //         }
                
        //         System.out.println("filename:" + path);
        //         System.out.println("key:" + key);
        //         System.out.println("row.tempo:" + row.tempo());
                
        //         RekordboxAnlz.BeatGridTag bgtag = findBeatGridTag(anlz);

        //         if (bgtag == null) {
        //             System.out.println("No beat grid");
        //         }
                
        //         printBeatGridTag(bgtag);
                
        //         ArrayList<RekordboxAnlz.CueTag> cuetags = findCueTag(anlz);
                
        //         ArrayList<RekordboxAnlz.CueEntry> ces = getSortedCueEntries(cuetags);
                
        //         for (RekordboxAnlz.CueEntry ce : ces) {
        //             printCueEntry(ce);
        //         }
                
        //         // int beatCount = (int)tag.lenBeats();
        //         // if (beatCount == 0) {
        //         //     System.out.println("beatCount is zero");
        //         //     continue;
        //         // }
        //         // int[] beatWithinBarValues = new int[beatCount];
        //         // int[] bpmValues = new int[beatCount];
        //         // long[] timeWithinTrackValues = new long[beatCount];
        //         // for (int beatNumber = 0; beatNumber < beatCount; beatNumber++) {
        //         //     RekordboxAnlz.BeatGridBeat beat = tag.beats().get(beatNumber);
        //         //     beatWithinBarValues[beatNumber] = beat.beatNumber();
        //         //     bpmValues[beatNumber] = beat.tempo();
        //         //     timeWithinTrackValues[beatNumber] = beat.time();
        //         // }
        //         // System.out.println("first beat ms:" + timeWithinTrackValues[0]);
        //     }
        //     System.out.println("size is " + map.size());
        // } catch (IOException e) {
        //     System.out.println(e);
        // }
    }
}
