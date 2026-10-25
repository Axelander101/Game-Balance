package ch.alexb.tower.balancebridge;

import ch.alexb.tower.Gegner.*;
import ch.alexb.tower.balance.SimpleJson;
import ch.alexb.tower.ui.Tower;

import java.awt.Point;
import java.lang.reflect.Field;
import java.nio.file.Path;
import java.util.*;

/**
 * Exportiert die wichtigsten Werte des bestehenden Java-Tower-Defense in das
 * generische JSON-Format der C++ Game Balance AI.
 */
public final class GeneralBalanceDataExporter {
    private GeneralBalanceDataExporter() {}

    public static void main(String[] args) throws Exception {
        Path root = args.length > 0 ? Path.of(args[0]).toAbsolutePath() : Path.of("").toAbsolutePath();
        Path out = export(root);
        System.out.println("Exportiert: " + out);
    }

    public static Path export(Path projectRoot) throws Exception {
        Path out = projectRoot.resolve("balance_data/tower_defense.json");
        Map<String,Object> root = new LinkedHashMap<>();
        root.put("name", "Tower Defense - Java Export");
        root.put("genre", "tower_defense");
        root.put("targets", Map.of("standardProgress", 180, "standardSoftMax", 202));
        root.put("rules", rules());
        root.put("towers", towers());
        root.put("enemies", enemies());
        SimpleJson.write(out, root);
        return out;
    }

    private static Map<String,Object> rules() {
        Map<String,Object> r = new LinkedHashMap<>();
        r.put("startMoney", 130);
        r.put("startLives", 30);
        r.put("bankCost", 250);
        r.put("bankIncome", 35);
        r.put("enemyCountBase", 5);
        r.put("enemyCountGrowth", 1.25);
        r.put("enemyCountQuadratic", 0.0045);
        r.put("enemyHpGrowth", 0.021);
        r.put("bossInterval", 10);
        r.put("maxTowerSlots", 46);
        r.put("pathPressure", 1.0);
        return r;
    }

    private static List<Object> towers() {
        String[] ids = {"", "normal", "sniper", "shotgun", "frost", "detector", "bomber", "moerser", "trapper", "village"};
        String[] roles = {"", "allround", "boss", "crowd", "control", "detection", "armor", "crowd", "crowd", "support"};
        List<Object> list = new ArrayList<>();
        for (int typ=1; typ<=9; typ++) {
            Tower t = new Tower();
            t.newTower(0,0,typ);
            double attacksPerSecond = 1000.0 / Math.max(1, t.getShootdelay());
            double power = t.getDemage() * attacksPerSecond * Math.max(1, t.getPearcing());
            if (t.isFlaechenschaden()) power *= 1.35;
            if (typ == 9) power = 12.0; // Support wird abstrakt bewertet.
            Map<String,Object> m = new LinkedHashMap<>();
            m.put("id", ids[typ]);
            m.put("role", roles[typ]);
            m.put("cost", t.getKosten(typ));
            m.put("power", Math.max(0.1, power));
            m.put("range", t.getReichweite());
            m.put("upgradeCost", Math.max(25, (int)Math.round(t.getKosten(typ) * 1.55)));
            list.add(m);
        }
        return list;
    }

    private static List<Object> enemies() throws Exception {
        ArrayList<Point> way = new ArrayList<>();
        way.add(new Point(0,0));
        way.add(new Point(100,0));
        List<EnemyMeta> defs = List.of(
                new EnemyMeta("normal","normal",1,5.0,new Enemy(way)),
                new EnemyMeta("fast","fast",5,2.0,new FastEnemy(way)),
                new EnemyMeta("strong","normal",15,1.5,new StrongEnemy(way)),
                new EnemyMeta("boss","boss",20,0.20,new Boss(way)),
                new EnemyMeta("invisible","hidden",25,0.75,new InvisEnemy(way)),
                new EnemyMeta("armored","armor",30,1.0,new ArmorEnemy(way)),
                new EnemyMeta("explosion_immune","normal",40,0.45,new ExplosionImuneEnemy(way)),
                new EnemyMeta("tarnpanzer","armor",65,0.45,new TarnPanzerEnemy(way)),
                new EnemyMeta("titan","boss",90,0.18,new Titan(way)),
                new EnemyMeta("fast_boss","boss",80,0.15,new FastBoss(way)),
                new EnemyMeta("endboss","boss",100,0.05,new EndBoss(way))
        );
        List<Object> out = new ArrayList<>();
        for (EnemyMeta d : defs) {
            Gegner e = d.enemy;
            Map<String,Object> m = new LinkedHashMap<>();
            m.put("id", d.id);
            m.put("role", d.role);
            m.put("hp", Math.max(1.0, e.getLeben()));
            m.put("speed", Math.max(0.1, readDouble(e, "geschwindigkeit")));
            int reward = e.getBelohnung();
            if (reward <= 0) reward = d.role.equals("boss") ? 100 : 5;
            m.put("reward", reward);
            m.put("share", d.share);
            m.put("firstWave", d.firstWave);
            out.add(m);
        }
        return out;
    }

    private static double readDouble(Object obj, String field) throws Exception {
        Field f = Gegner.class.getDeclaredField(field);
        f.setAccessible(true);
        return ((Number)f.get(obj)).doubleValue();
    }

    private record EnemyMeta(String id, String role, int firstWave, double share, Gegner enemy) {}
}
