package ch.alexb.tower.balancebridge;

import java.io.IOException;
import java.nio.file.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.List;

/** Startet die kompilierte C++ Balance AI aus dem Java-Projekt. */
public final class GeneralBalanceLauncher {
    private GeneralBalanceLauncher() {}

    public static void main(String[] args) throws Exception {
        Path root = args.length > 0 ? Path.of(args[0]).toAbsolutePath() : Path.of("").toAbsolutePath();
        Process p = start(root, false);
        int code = p.waitFor();
        System.out.println("General Balance AI beendet mit Code " + code);
        if (code != 0) System.exit(code);
    }

    public static Process start(Path projectRoot, boolean resume) throws Exception {
        Path gameJson = GeneralBalanceDataExporter.export(projectRoot);
        Path cppRoot = projectRoot.resolve("balance_ai_cpp");
        Path executable = findExecutable(cppRoot);
        Path config = cppRoot.resolve("config/tower_defense_3h.json");
        String stamp = DateTimeFormatter.ofPattern("yyyyMMdd_HHmmss").format(LocalDateTime.now());
        Path out = projectRoot.resolve("general_balance_runs/td_" + stamp);
        Files.createDirectories(out);

        ProcessBuilder pb = new ProcessBuilder(
                executable.toString(),
                "--game", gameJson.toString(),
                "--config", config.toString(),
                "--out", out.toString()
        );
        if (resume) pb.command().add("--resume");
        pb.directory(projectRoot.toFile());
        pb.inheritIO();
        return pb.start();
    }

    public static Path findExecutable(Path cppRoot) throws IOException {
        List<Path> candidates = List.of(
                cppRoot.resolve("cmake-build-release/game_balance_ai"),
                cppRoot.resolve("cmake-build-debug/game_balance_ai"),
                cppRoot.resolve("build/game_balance_ai"),
                cppRoot.resolve("cmake-build-relwithdebinfo/game_balance_ai")
        );
        for (Path p : candidates) if (Files.isRegularFile(p) && Files.isExecutable(p)) return p;
        throw new IOException("C++ Balance AI wurde noch nicht gebaut. Erwartet z.B.: " + candidates.get(0));
    }
}
