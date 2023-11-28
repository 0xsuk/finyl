package com.example;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.deepsymmetry.cratedigger.Database;
import org.deepsymmetry.cratedigger.pdb.*;
import io.kaitai.struct.RandomAccessFileKaitaiStream;
import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.io.FileWriter;
import java.io.BufferedWriter;
import java.io.IOException;
import java.util.*;

class Beat {
    public long tempo; //for dynamic tempo?
    public long time;
    public int num; //1 to 4

    public Beat(long tempo, long time, int num) {
        this.tempo = tempo;
        this.time = time;
        this.num = num;
    }
}

class Cue {
    public long type; //memory cue or loop
    public long time;

    public Cue(long type, long time) {
        this.type = type;
        this.time = time;
    }
}

class TrackDetail {
    public Track t;
    public ArrayList<Cue> cues;
    public ArrayList<Beat> beats;
    
}

class Track {
    public long id;
    public long musickeyid;
    public long tempo;
    public long filesize;
    public String filepath;
    public String filename;
    public String title;
    public String anlzPath;
    
    public Track(RekordboxPdb.TrackRow tr)  {
        this.id = tr.id();
        this.musickeyid = tr.keyId();
        this.tempo = tr.tempo();
        this.filesize = tr.fileSize();
        this.filepath = Database.getText(tr.filePath());
        this.filename = Database.getText(tr.filename());
        this.title = Database.getText(tr.title());
        this.anlzPath = getAnlzPath(tr);
    }
    
    public static void printBeatGridBeat(RekordboxAnlz.BeatGridBeat bgbeat) {
        System.out.println("\tbeatNumber:" + bgbeat.beatNumber());
        System.out.println("\ttempo:" + bgbeat.tempo());
        System.out.println("\ttime:" + bgbeat.time());
    }
    
    public static void printBeatGridTag(RekordboxAnlz.BeatGridTag bgtag) {
        
        System.out.println("lenBeats:" + bgtag.lenBeats());
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
            for (RekordboxAnlz.CueEntry entry : cuetag.cues()) {
                printCueEntry(entry);
            }
        }
    }
    
    public static RekordboxPdb.TrackRow getTrackRow(long id) {
        Map<Long, RekordboxPdb.TrackRow> map = App.database.trackIndex;
        for (Map.Entry<Long, RekordboxPdb.TrackRow> entry : map.entrySet()) {
            long _id = entry.getKey();
            if (id == _id) {
                return entry.getValue();
            }
        }
        return null;
    }
    
    public static String getAnlzPath(RekordboxPdb.TrackRow tr) {
        return new File(App.usb, Database.getText(tr.analyzePath())).getAbsolutePath();
    }
    
    public static RekordboxAnlz getAnlz(RekordboxPdb.TrackRow tr) {
        String anlzpath = getAnlzPath(tr);
        try {
            return new RekordboxAnlz(new RandomAccessFileKaitaiStream(anlzpath));
        } catch(IOException e) {
            return null;
        }
    }
    
    public static RekordboxAnlz.BeatGridTag findBeatGridTag(RekordboxPdb.TrackRow tr) {
        RekordboxAnlz anlz = getAnlz(tr);
        for (RekordboxAnlz.TaggedSection section : anlz.sections()) {
            if (section.body() instanceof RekordboxAnlz.BeatGridTag) {
                return (RekordboxAnlz.BeatGridTag) section.body();
            }
        }
        return null;
    }

    public static ArrayList<Beat> getBeats(RekordboxPdb.TrackRow tr) {
        ArrayList<Beat> beats = new ArrayList<>();
        RekordboxAnlz.BeatGridTag bgtag = findBeatGridTag(tr);
        if (bgtag == null) {
            // no beat information
            return beats;
        }
        
        for (RekordboxAnlz.BeatGridBeat beat : bgtag.beats()) {
            beats.add(new Beat(beat.tempo(), beat.time(), beat.beatNumber()));
        }
        return beats;
    }
    
    public static ArrayList<RekordboxAnlz.CueTag> findCueTag(RekordboxPdb.TrackRow tr) {
        RekordboxAnlz anlz = getAnlz(tr);
        ArrayList<RekordboxAnlz.CueTag> cuetags = new ArrayList<>();
        for (RekordboxAnlz.TaggedSection section : anlz.sections()) {
            if (section.body() instanceof RekordboxAnlz.CueTag) {
                RekordboxAnlz.CueTag cuetag = (RekordboxAnlz.CueTag) section.body();
                cuetags.add(cuetag);
            }
        }
        
        return cuetags;
    }

    public static ArrayList<Cue> findCues(RekordboxPdb.TrackRow tr) {
        ArrayList<RekordboxAnlz.CueTag> cuetags = findCueTag(tr);
        ArrayList<Cue> cues = new ArrayList<>();
        for (RekordboxAnlz.CueTag cuetag : cuetags) {
            if (cuetag.lenCues() > 0) {
                for (RekordboxAnlz.CueEntry ce : cuetag.cues()) {
                    cues.add(new Cue(ce.type().id(), ce.time()));
                }
            }
        }

        return cues;
    }
    
    public static void sortCues(ArrayList<Cue> cues) {
        Collections.sort(cues, (cue1, cue2) -> Long.compare(cue1.time, cue2.time));
    }

    
    public static ArrayList<Cue> getSortedCues(RekordboxPdb.TrackRow tr) {
        ArrayList<Cue> cues = findCues(tr);
        sortCues(cues);
        return cues;
    }
}

class Out {
    static private BufferedWriter bw;
    private static ObjectMapper objm = new ObjectMapper();
    public String error = null;
    public Map<Long, String> playlists = new HashMap<>(); //id to playlist name
    public ArrayList<Track> tracks = new ArrayList<>();
    
    public Out() {
    }

    public static void open() {
        try {
            String home = System.getProperty("user.home");
            Path f = Path.of(home, ".finyl-output");
            bw = new BufferedWriter(new FileWriter(f.toFile()));
        } catch (Exception e) {
            System.out.println("failed to open output file:" + e);
            System.exit(1);
        }
    }
    
    public static void writeTrack() {
        Out out = new Out();
    }
    
    public static void writePlaylists() {
        Out out = new Out();
        
        Map<Long, List<Database.PlaylistFolderEntry>> pfindex = App.database.playlistFolderIndex;
        //basically pfindex is map of single set
        for (Map.Entry<Long, List<Database.PlaylistFolderEntry>> entry : pfindex.entrySet()) {
            for (Database.PlaylistFolderEntry e : entry.getValue()) {
                out.playlists.put(e.id, e.name);
            }
        }
        
        out.write();
        System.exit(0);
    }
    
    public static void writeAllTracks() {
        Out out = new Out();
        Map<Long, RekordboxPdb.TrackRow> map = App.database.trackIndex;
        for (Map.Entry<Long, RekordboxPdb.TrackRow> entry : map.entrySet()) {
            RekordboxPdb.TrackRow tr = entry.getValue();
            Track t = new Track(tr);
            out.tracks.add(t);
        }
        out.write();
        System.exit(0);
    }
    
    public static void writeError(String error) {
        Out out = new Out();
        out.error = error;
        out.write();
        System.exit(0);
    }
    
    public void write() {
        try {
            bw.write(objm.writeValueAsString(this));
            bw.close(); //without this, no text is written
        } catch (IOException e) {
            System.out.println("Failed to write:" + e);
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
    static public File usb;
    static public File pdb;
    static public String op;
    static public ArrayList<String> params;
    static public Database database;
    
    public static void parseArgs(String[] args) {
        must(args.length >= 2);
        usb = new File(args[0]);
        if (!usb.exists()) {
            Out.writeError("usb not found");
        }
        pdb = new File(usb, "/PIONEER/rekordbox/export.pdb");
        if (!pdb.exists()) {
            Out.writeError("pdb not found for this usb");
        }
        op = args[1];
        if (args.length > 2) {
            params = new ArrayList<>(Arrays.asList(Arrays.copyOfRange(args, 2, args.length)));
        }
        try {
            database = new Database(pdb);
        } catch (IOException e) {
            Out.writeError("failed to read database");
        }
    }
    
    public static void must(Boolean test) {
        if (!test) {
            Out.writeError("assertion failed");
        }
    }
    
    public static void main(String[] args) {
        Out.open();
        parseArgs(args);
        switch (op) {
        case "playlists":
            Out.writePlaylists();
        case "all-tracks":
            Out.writeAllTracks();
        default:
            must(false);
        }
    }
}
