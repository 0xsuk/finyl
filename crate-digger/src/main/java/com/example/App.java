package com.example;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.deepsymmetry.cratedigger.Database;
import org.deepsymmetry.cratedigger.pdb.*;
import io.kaitai.struct.RandomAccessFileKaitaiStream;
import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.stream.Collectors;
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

class Playlist {
    public long id;
    public String name;

    public Playlist(long id, String name) {
        this.id = id;
        this.name = name;
    }
}

class TrackDetail {
    public Track t;
    public ArrayList<Cue> cues;
    public ArrayList<Beat> beats;

    public TrackDetail(RekordboxPdb.TrackRow tr) {
        this.t = new Track(tr);
        this.cues = Track.getSortedCues(tr);
        this.beats = Track.getBeats(tr);
    }
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
        this.filepath = getFilePath(tr);
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
    
    public static String getFilePath(RekordboxPdb.TrackRow tr) {
        return new File(App.usb, Database.getText(tr.filePath())).getAbsolutePath();
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
    
    public static boolean trackExist(long id) {
        RekordboxPdb.TrackRow tr = Track.getTrackRow(id);
        if (tr == null) {
            return false;
        }
        return true;
    }
    
    public static ArrayList<Track> findTracks(long playlistid) {
        for (Map.Entry<Long, List<Long>> entry : App.database.playlistIndex.entrySet()) {
            long pid = entry.getKey();
            if (pid == playlistid) {
                List<Long> tids = new ArrayList<Long>(entry.getValue());
                tids = tids.stream().filter(tid -> trackExist(tid)).collect(Collectors.toList());
                List<Track> tracks = tids.stream().map(tid -> new Track(getTrackRow(tid))).collect(Collectors.toList());
                return new ArrayList<>(tracks);
            }
        }

        return null;
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
    private static BufferedWriter bw;
    private static ObjectMapper objm = new ObjectMapper();
    public String usb = "";
    public String badge = "";
    public String error = null;
    public ArrayList<Playlist> playlists = new ArrayList<>(); //id to playlist name
    public ArrayList<Track> tracks = new ArrayList<>();
    public TrackDetail track = null;
    
    public Out() {
        try {
            String home = System.getProperty("user.home");
            Path f = Path.of(home, ".finyl-output");
            bw = new BufferedWriter(new FileWriter(f.toFile()));
            usb = App.usb.getAbsolutePath();
            badge = App.badge;
        } catch (Exception e) {
            System.out.println("failed to open output file:" + e);
            System.exit(1);
        }
            
    }

    public static void open() {
    }
    
    //trakcs in playlist
    public static void writePlaylistTracks() {
        App.must(App.params.size() > 0);
        String pid = App.params.get(0);
        
        try {
            long id = Long.parseLong(pid);
            ArrayList<Track> tracks = Track.findTracks(id);
            if (tracks == null) {
                writeError("Playlist not found for id =" + id);
            }
            App.out.tracks = tracks;
        } catch (Exception e) {
            writeError("not valid playlist id = " + pid);
        }

        App.out.write();
    }
    
    public static void writeTrack() {
        App.must(App.params.size() > 0);
        String tid = App.params.get(0);
        
        try {
            long id = Long.parseLong(tid);
            RekordboxPdb.TrackRow tr = Track.getTrackRow(id);
            if (tr == null) {
                writeError("Track not found for id = " + tid);
            }
            App.out.track = new TrackDetail(tr);
            
        } catch (Exception e){
            writeError("not valid track id = " + tid);
        }

        App.out.write();
    }
    
    public static void writePlaylists() {
        Map<Long, List<Database.PlaylistFolderEntry>> pfindex = App.database.playlistFolderIndex;
        //basically pfindex is map of single set
        for (Map.Entry<Long, List<Database.PlaylistFolderEntry>> entry : pfindex.entrySet()) {
            for (Database.PlaylistFolderEntry e : entry.getValue()) {
                if (e == null) {
                    System.out.println("null entry");
                    continue;
                }
                Playlist p = new Playlist(e.id, e.name);
                App.out.playlists.add(p);
            }
        }
        
        App.out.write();
    }
    
    public static void writeAllTracks() {
        Map<Long, RekordboxPdb.TrackRow> map = App.database.trackIndex;
        for (Map.Entry<Long, RekordboxPdb.TrackRow> entry : map.entrySet()) {
            RekordboxPdb.TrackRow tr = entry.getValue();
            Track t = new Track(tr);
            App.out.tracks.add(t);
        }
        App.out.write();
    }
    
    public static void writeError(String error) {
        App.out.error = error;
        App.out.write();
    }
    
    public void write() {
        try {
            bw.write(objm.writeValueAsString(this));
            bw.close(); //without this, no text is written
        } catch (IOException e) {
            System.out.println("Failed to write:" + e);
            System.exit(1);
        }
        System.exit(0);
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
    public static String badge; //caller of this program can ensure that output file (.finyl-output) is updated to latest call, by providing unique string as badge to each call.
    static public String op = "";
    static public ArrayList<String> params = new ArrayList<>();
    static public Database database;
    static Out out;
    
    public static void parseArgs(String[] args) {
        if (!(args.length >= 3)) { //badge, usb, op
            fatal("arg.length must be >= 3");
        }
        
        badge = args[0];
        
        usb = new File(args[1]);
        if (!usb.exists()) {
            fatal("usb not found");
        }
        pdb = new File(usb, "/PIONEER/rekordbox/export.pdb");
        if (!pdb.exists()) {
            fatal("pdb not found for this usb");
        }
        op = args[2];
        if (args.length > 3) {
            params = new ArrayList<>(Arrays.asList(Arrays.copyOfRange(args, 3, args.length)));
        }
        try {
            database = new Database(pdb);
        } catch (IOException e) {
            fatal("failed to read database");
        }
    }
    
    public static void must(Boolean test) {
        if (!test) {
            Out.writeError("assertion failed");
        }
    }
    
    public static void fatal(String message) {
        System.out.println(message);
        System.exit(1);
    }
    
    public static void main(String[] args) {
        parseArgs(args);
        out = new Out();
        switch (op) {
        case "playlists":
            Out.writePlaylists();
        case "all-tracks":
            Out.writeAllTracks();
        case "track":
            Out.writeTrack();
        case "playlist-tracks":
            Out.writePlaylistTracks();
        default:
            Out.writeError("invalid operation op = " + op);
        }
    }
}
