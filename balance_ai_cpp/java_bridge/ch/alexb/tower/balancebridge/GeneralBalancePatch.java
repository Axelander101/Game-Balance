package ch.alexb.tower.balancebridge;

import ch.alexb.tower.balance.SimpleJson;
import java.nio.file.Path;
import java.util.*;

/** Liest die von C++ erzeugten Balance-Gene ein. */
public final class GeneralBalancePatch {
    private GeneralBalancePatch() {}

    public static Map<String,Double> load(Path currentBalanceJson) throws Exception {
        Map<String,Object> root = SimpleJson.object(SimpleJson.read(currentBalanceJson));
        Map<String,Object> raw = SimpleJson.object(root.get("genes"));
        Map<String,Double> out = new LinkedHashMap<>();
        for (var e : raw.entrySet()) out.put(e.getKey(), SimpleJson.number(e.getValue(), 1.0));
        return out;
    }
}
