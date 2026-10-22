/* Browser UI only. Native-compatible calculations live in measure-engine.js. */
(function () {
  "use strict";
  const MAX_BYTES = 256;
  const encoder = new TextEncoder();
  function utf8Prefix(value, available = MAX_BYTES) {
    let result = "", length = 0;
    for (const character of value) {
      const size = encoder.encode(character).length;
      if (length + size > available) break;
      result += character; length += size;
    }
    return result;
  }
  function fitInsertion(value, start, end, insertion) {
    const before = value.slice(0, start), after = value.slice(end);
    const available = MAX_BYTES - encoder.encode(before + after).length;
    return utf8Prefix(insertion, available);
  }
  if (typeof module === "object" && module.exports) module.exports = { utf8Prefix, fitInsertion };
  if (typeof document === "undefined") return;
  function initialize() {
    document.querySelectorAll("[data-print]").forEach(button => {
      if (button.dataset.initialized) return;
      button.dataset.initialized = "true";
      button.addEventListener("click", () => window.print());
    });
    const home = document.querySelector(".lz-home");
    if (!home || home.dataset.initialized || typeof LZMeasures === "undefined") return;
    home.dataset.initialized = "true";
    const { analyze, emc, distance } = LZMeasures;
    const get = id => home.querySelector("#" + id);
    const input = get("lz-input"), secondInput = get("lz-input-b"), error = get("lz-input-error");
    const tabs = [...home.querySelectorAll("[data-mode]")];
    let mode = "parse";
    function byteCount() {
      get("lz-byte-count").textContent = encoder.encode(input.value).length + " / 256 bytes";
      get("lz-byte-count-b").textContent = encoder.encode(secondInput.value).length + " / 256 bytes";
    }
    function renderParse(result) {
      const parse = get("lz-parse"), decoder = new TextDecoder();
      parse.replaceChildren();
      for (let i = 0; i < result.boundaries.length - 1; i++) {
        const start = result.boundaries[i], end = Math.min(result.boundaries[i + 1], result.bytes.length);
        if (end <= start) continue;
        const tile = document.createElement("span"), value = result.bytes.slice(start, end);
        // Hex preserves byte boundaries when a parse splits a UTF-8 character.
        tile.textContent = value.some(byte => byte >= 128) ? Array.from(value, byte => byte.toString(16).padStart(2, "0")).join(" ") : decoder.decode(value).replace(/ /g, "␠").replace(/\n/g, "↵").replace(/\t/g, "⇥");
        tile.title = "Bytes " + start + "–" + end;
        parse.append(tile);
      }
      get("lz-demo-count").textContent = result.count;
      get("lz-demo-h").textContent = result.h.toFixed(4);
      get("lz-h-label").textContent = "Entropy density h · base " + result.logBase;
      let description = "The final copied tail can be excluded from the reported count. Short-sequence estimates of h can exceed one.";
      if (!result.bytes.length) description = "Empty input: count 1, h 0. Enter a sequence to inspect its factors.";
      else if (input.value === "banana") description = "The count is 3: the final copied tail adds no new factor. Short-sequence estimates of h can exceed one.";
      else if (result.bytes.some(value => value >= 128)) description += " Non-ASCII pieces are shown as UTF-8 byte values.";
      get("lz-lab-description").textContent = description;
    }
    let curveValues = [], curveWidth = 0;
    function renderCurve(values) {
      const chart = get("lz-e-chart"), ns = "http://www.w3.org/2000/svg";
      curveValues = values;
      curveWidth = Math.max(180, Math.round(chart.getBoundingClientRect().width));
      chart.setAttribute("viewBox", "0 0 " + curveWidth + " 180");
      chart.replaceChildren();
      const left = 36, right = curveWidth - 10, top = 16, bottom = 146;
      const peak = Math.max(...values, 1), last = Math.max(values.length - 1, 1);
      function shape(tag, attributes, text) {
        const node = document.createElementNS(ns, tag);
        Object.entries(attributes).forEach(([key, value]) => node.setAttribute(key, String(value)));
        if (text !== undefined) node.textContent = text;
        chart.append(node); return node;
      }
      [0, .5, 1].forEach(fraction => {
        const y = bottom - fraction * (bottom - top);
        shape("line", { x1:left, y1:y, x2:right, y2:y, class:"lz-chart-grid" });
        shape("text", { x:left - 8, y:y + 4, "text-anchor":"end" }, (peak * fraction).toFixed(1));
      });
      const points = values.map((value, index) => [left + index / last * (right - left), bottom - value / peak * (bottom - top)]);
      const path = points.map(([x, y], index) => (index ? "L" : "M") + x.toFixed(2) + " " + y.toFixed(2)).join(" ");
      if (points.length) {
        shape("path", { d:path + " L" + points[points.length - 1][0] + " " + bottom + " L" + left + " " + bottom + " Z", class:"lz-chart-fill" });
        shape("path", { d:path, class:"lz-chart-line" });
        points.forEach(([cx, cy]) => shape("circle", {cx, cy, r:2.5}));
      }
      shape("text", {x:left, y:bottom + 22, "text-anchor":"middle"}, "1");
      if (values.length > 1) shape("text", {x:right, y:bottom + 22, "text-anchor":"end"}, String(values.length));
      shape("text", {x:(left + right) / 2, y:bottom + 22, "text-anchor":"middle"}, "Range (bytes)");
      chart.setAttribute("aria-label", "E profile at ranges 1 to " + values.length + ", ending at " + (values[values.length - 1] || 0).toFixed(4));
    }
    function update() {
      byteCount();
      input.removeAttribute("aria-invalid"); secondInput.removeAttribute("aria-invalid"); error.hidden = true; error.textContent = ""; get("lz-result-state").hidden = true;
      // Hide previous output on invalid input, so an old number cannot be mistaken for a new result.
      const panel = get("lz-panel-" + mode);
      try {
        const result = analyze(input.value);
        get("lz-normalization").textContent = " · automatic logarithm base " + result.logBase;
        get("lz-normalization").hidden = mode === "distance";
        panel.hidden = false;
        if (mode === "parse") renderParse(result);
        if (mode === "structure") {
          const structure = emc(input.value);
          get("lz-demo-e").textContent = structure.value.toFixed(4);
          get("lz-demo-range").textContent = structure.maxBlockSize;
          renderCurve(structure.fitted);
          let description = "E across ranges. Try sequences of the same length to compare their organization.";
          if (result.bytes.length <= 10) description = "This short sequence has a flat E profile. Try Periodic to explore a longer example.";
          else if (result.bytes.every(byte => byte === result.bytes[0])) description = "A constant sequence has E = 0. Try Periodic to see a nonzero range profile.";
          get("lz-e-description").textContent = description;
        }
        if (mode === "distance") {
          const comparison = distance(input.value, secondInput.value);
          get("lz-demo-distance").textContent = comparison.value.toFixed(4);
        }
        panel.hidden = false;
        get("lz-lab-method").hidden = false;
      } catch (failure) {
        error.textContent = failure.message;
        error.hidden = false;
        get("lz-result-state").textContent = "Check the sequence length, then analyze again."; get("lz-result-state").hidden = false;
        if (encoder.encode(input.value).length > 256) input.setAttribute("aria-invalid", "true");
        if (mode === "distance" && encoder.encode(secondInput.value).length > 256) secondInput.setAttribute("aria-invalid", "true");
        panel.hidden = true;
        get("lz-lab-method").hidden = true;
      }
    }
    function selectMode(next) {
      mode = next;
      tabs.forEach(tab => {
        const selected = tab.dataset.mode === mode;
        tab.setAttribute("aria-selected", String(selected)); tab.tabIndex = selected ? 0 : -1;
        get("lz-panel-" + tab.dataset.mode).hidden = !selected;
      });
      get("lz-second-input").hidden = mode !== "distance";
      get("lz-input-label").textContent = mode === "distance" ? "Sequence A" : "Your sequence";
      get("lz-analyze").firstChild.textContent = mode === "distance" ? "Compare sequences " : "Analyze sequence ";
      update();
    }
    tabs.forEach((tab, index) => {
      tab.addEventListener("click", () => selectMode(tab.dataset.mode));
      tab.addEventListener("keydown", event => {
        let next;
        if (event.key === "ArrowRight") next = (index + 1) % tabs.length;
        if (event.key === "ArrowLeft") next = (index + tabs.length - 1) % tabs.length;
        if (event.key === "Home") next = 0;
        if (event.key === "End") next = tabs.length - 1;
        if (next !== undefined) { event.preventDefault(); tabs[next].focus(); selectMode(tabs[next].dataset.mode); }
      });
    });
    function irregular(length) {
      let state = 71423, result = "";
      for (let i = 0; i < length; i++) {
        state ^= state << 13; state ^= state >>> 17; state ^= state << 5;
        result += String((state >>> 0) & 1);
      }
      return result;
    }
    // Escherichia phage lambda, J02459.1, bases 1–256.
    // https://www.ebi.ac.uk/ena/browser/api/fasta/J02459.1?download=false
    const dna =
      "GGGCGGCGACCTCGCGGGTTTTCGCTATTTATGAAAATTTTCCGGTTTAAGGCGTTTCCGTTCT" +
      "TCTTCGTCATAACTTAATGTTTTTATTTAAAATACCCTCTGAAAAGAAAGGAAACGACAGGTGC" +
      "TGAAAGCGAGGCTTTTTGGCCTCTGTCGTTTCCTTTCTCTGTTTTTGTCCGTGGAATGAACAAT" +
      "GGAAGTCAACAAAAAGCAGCTGGCTGACATTTTCGGTGCGAGTATCCGTACCATTCAGAACTGG";
    const relatedDna = dna.split("");
    relatedDna[63] = "C";
    relatedDna[127] = "T";
    relatedDna[191] = "C";
    const examples = {
      banana: ["banana", "banaba", "A short example for following the LZ76 factor boundaries."],
      periodic: ["00010111".repeat(32), "00010111".repeat(31) + "00010110", "A 256-byte sequence built from an eight-symbol repeating motif."],
      irregular: [irregular(256), irregular(256).split("").reverse().join(""), "A fixed pseudorandom example. Irregularity alone does not imply rich organization."],
      constant: ["0".repeat(256), "0".repeat(256), "A constant 256-byte sequence. Compare it with Periodic and Irregular."],
      dna: [dna, relatedDna.join(""), "First 256 bases of bacteriophage λ (GenBank J02459.1). Sequence B has three substitutions."]
    };
    home.querySelectorAll("[data-example]").forEach(button => button.addEventListener("click", () => {
      const [first, second, note] = examples[button.dataset.example];
      input.value = first; secondInput.value = second; get("lz-example-note").textContent = note; update();
    }));
    get("lz-analyze").addEventListener("click", update);
    [input, secondInput].forEach(field => {
      function insertWithinLimit(event, text) {
        const start = field.selectionStart, end = field.selectionEnd;
        const accepted = fitInsertion(field.value, start, end, text);
        if (accepted === text) return;
        event.preventDefault();
        if (!accepted) return;
        field.setRangeText(accepted, start, end, "end");
        field.dispatchEvent(new Event("input", { bubbles: true }));
      }
      field.addEventListener("beforeinput", event => {
        if (event.cancelable && !event.isComposing && event.inputType.startsWith("insert")) {
          const text = event.data ??
            (["insertLineBreak", "insertParagraph"].includes(event.inputType) ? "\n" : event.dataTransfer?.getData("text/plain"));
          if (text !== undefined && text !== null) insertWithinLimit(event, text);
        }
      });
      field.addEventListener("paste", event => {
        if (event.clipboardData) insertWithinLimit(event, event.clipboardData.getData("text/plain"));
      });
      function enforceLimit() {
        const limited = utf8Prefix(field.value);
        if (limited !== field.value) {
          const caret = Math.min(field.selectionStart, limited.length);
          field.value = limited;
          field.setSelectionRange(caret, caret);
        }
      }
      field.addEventListener("compositionend", () => {
        enforceLimit();
        field.dispatchEvent(new Event("input", { bubbles: true }));
      });
      field.addEventListener("input", event => {
        if (event.isComposing) return;
        enforceLimit();
        byteCount();
        error.hidden = true; error.textContent = ""; field.removeAttribute("aria-invalid");
        get("lz-result-state").textContent = "Ready to analyze your new sequence."; get("lz-result-state").hidden = false;
        get("lz-example-note").textContent = "Press " + (mode === "distance" ? "Compare sequences" : "Analyze sequence") + " to update the result. Ctrl/⌘ + Enter also works.";
        // Avoid leaving stale results visible while a different sequence is being edited.
        get("lz-panel-" + mode).hidden = true; get("lz-lab-method").hidden = true;
      });
      field.addEventListener("keydown", event => { if (event.key === "Enter" && (event.ctrlKey || event.metaKey)) { event.preventDefault(); update(); } });
    });
    home.querySelectorAll("[data-copy]").forEach(button => button.addEventListener("click", async () => {
      const original = button.textContent;
      try { await navigator.clipboard.writeText(button.dataset.copy); button.textContent = "Copied ✓"; }
      catch (_) { button.textContent = "Select command to copy"; }
      setTimeout(() => { if (button.isConnected) button.textContent = original; }, 1800);
    }));
    if (typeof ResizeObserver !== "undefined") new ResizeObserver(() => {
      if (mode === "structure" && !get("lz-panel-structure").hidden && curveValues.length &&
          Math.abs(get("lz-e-chart").getBoundingClientRect().width - curveWidth) > 1) renderCurve(curveValues);
    }).observe(get("lz-e-chart"));
    update();
  }
  if (typeof document$ !== "undefined") document$.subscribe(initialize);
  else if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", initialize);
  else initialize();
})();
