---
template: home.html
title: lzcomplexity
description: Explore symbolic sequences with LZ76 factorization, entropy-rate estimation, shuffle-based complexity and information distance. A Rust core, Python API and two command-line tools from Pleros.
search:
  exclude: true
hide:
  - navigation
  - toc
---

<div class="lz-home">
<section class="lz-hero">
<div class="lz-hero-copy">
<p class="lz-eyebrow"><span class="lz-status"></span> PLEROS · SCIENTIFIC COMPUTING</p>
<h1>Find the structure.<br><em>Understand<br class="lz-mobile-break"> the noise.</em></h1>
<p class="lz-hero-lead">A sequence is more than its symbols. Explore what repeats, what surprises, and what changes when you shuffle it.</p>
<p class="lz-hero-description"><strong>lzcomplexity</strong> brings Lempel–Ziv 76 analysis to Python, Rust and the command line.</p>
<div class="lz-actions"><a class="lz-button lz-button-primary" href="guide/install/">Start exploring <span aria-hidden="true">↗</span></a><a class="lz-text-link" href="api/python/">Read the API <span aria-hidden="true">→</span></a></div>
<div class="lz-install"><code>pip install lzcomplexity</code><button type="button" data-copy="pip install lzcomplexity" aria-label="Copy installation command">Copy <span aria-hidden="true">⧉</span></button></div>
<p class="lz-tiny">Open source · MIT licensed · Built by the Pleros Group</p>
</div>
<div class="lz-specimen" aria-label="Interactive view of a symbolic sequence">
<div class="lz-specimen-head"><span><i></i> A CLOSER LOOK</span><span>64 SYMBOLS</span></div>
<div class="lz-specimen-tabs" role="group" aria-label="Choose a sequence"><button type="button" data-specimen="periodic" aria-pressed="true">Repeating</button><button type="button" data-specimen="irregular" aria-pressed="false">Irregular</button></div>
<div class="lz-symbol-grid" id="lz-symbol-grid" aria-hidden="true"></div>
<div class="lz-specimen-readout"><div><span class="lz-metric-label">LZ76 COUNT</span><strong id="lz-specimen-count">2</strong></div><div><span class="lz-metric-label">ENTROPY DENSITY</span><strong id="lz-specimen-h">0.188<span> / byte</span></strong></div></div>
<p class="lz-specimen-note" id="lz-specimen-note">Repeated patterns need fewer new pieces.</p>
<div class="lz-specimen-foot"><span>log base 2 · finite-length estimate</span><a href="concepts/lz76/" aria-label="Learn how LZ76 works">How it works ↗</a></div>
<noscript><p>For the repeating sequence <code>01</code> × 32, the library reports LZ76 count 2 and entropy density 0.1875 with base-two logarithms.</p></noscript>
</div>
</section>

<div class="lz-proof-strip"><span>ONE RUST CORE</span><span>Python <span aria-hidden="true">↗</span></span><span>Command line <span aria-hidden="true">↗</span></span><span>Native Rust <span aria-hidden="true">↗</span></span><span class="lz-proof-end">Your data. Your questions.</span></div>

<section class="lz-section lz-measures" id="explore">
<div class="lz-section-heading"><p class="lz-eyebrow">01 / THE MEASURES</p><h2>Different questions.<br><em>A common foundation.</em></h2><p>Start with the LZ76 factor count. Use it to explore unpredictability, the response to shuffling, or the difference between two sequences.</p></div>
<div class="lz-measure-list">
<a class="lz-measure" href="concepts/entropy-density/"><span class="lz-measure-symbol">h</span><div><h3>How unpredictable is it?</h3><p>Estimate entropy density from the sequence’s LZ76 factor count and length. Order matters, not just symbol frequencies.</p></div><span class="lz-measure-arrow" aria-hidden="true">↗</span></a>
<a class="lz-measure" href="concepts/emc/"><span class="lz-measure-symbol">E<span>*</span></span><div><h3>What does shuffling reveal?</h3><p>Compare the original with block-shuffled copies at different scales. An EMC-like measure of organization, with its own definition and limits.</p></div><span class="lz-measure-arrow" aria-hidden="true">↗</span></a>
<a class="lz-measure" href="concepts/nid/"><span class="lz-measure-symbol">d</span><div><h3>How different are they?</h3><p>Compare two symbolic sequences using LZ76 counts of each sequence and their concatenations.</p></div><span class="lz-measure-arrow" aria-hidden="true">↗</span></a>
</div>
</section>

<section class="lz-lab" id="try-it">
<div class="lz-lab-intro"><p class="lz-eyebrow">02 / THE SEQUENCE LAB</p><h2>See the pieces.<br><em>Make the connection.</em></h2><p>LZ76 walks through a sequence and identifies pieces using what appeared earlier. Try a short sequence and inspect the boundaries.</p><a class="lz-text-link" href="guide/first-factorization/">Walk through a complete example →</a></div>
<div class="lz-lab-workbench">
<label for="lz-input">YOUR SEQUENCE <span>up to 128 UTF-8 bytes</span></label>
<div class="lz-input-row"><input id="lz-input" value="banana" maxlength="128" spellcheck="false" autocomplete="off" aria-describedby="lz-lab-description"><button type="button" id="lz-analyze" aria-label="Analyze sequence">Analyze ↗</button></div>
<div class="lz-presets" role="group" aria-label="Example sequences"><span>TRY</span><button type="button" data-sequence="banana">banana</button><button type="button" data-sequence="0101010101010101">Repeating</button><button type="button" data-sequence="1001110110101000">Binary</button><button type="button" data-sequence="ACGTACGTACGT">DNA alphabet</button></div>
<div class="lz-parse" id="lz-parse" aria-label="Pieces of the sequence"><span>b</span><span>a</span><span>n</span><span>ana</span></div>
<div class="lz-lab-results" aria-live="polite" aria-atomic="true"><p><strong id="lz-demo-count">3</strong> <span>reported LZ76 count</span></p><p><strong id="lz-demo-h">1.2925</strong> <span>entropy density · base 2</span></p></div>
<p class="lz-lab-description" id="lz-lab-description">For <code>banana</code>, four displayed pieces give a reported count of three: the final copied tail is excluded by the library’s counting convention. Finite-length entropy estimates can exceed one.</p>
<noscript><p>Install the Python library to run your own sequences. The first-factorization guide explains this example step by step.</p></noscript>
</div>
</section>

<section class="lz-section lz-get-started">
<div class="lz-start-copy"><p class="lz-eyebrow">03 / FROM QUESTION TO CODE</p><h2>A small API.<br><em>Room to explore.</em></h2><p>Five Python functions cover factorization, entropy density, the shuffle-based score, sequence distance and a complete analysis.</p><div class="lz-interface-links"><a href="api/python/">Python reference <span>↗</span></a><a href="cli/">Command-line tools <span>↗</span></a><a href="api/rust/">Rust core <span>↗</span></a></div></div>
<div class="lz-code-window"><div class="lz-code-head"><span><i></i><i></i><i></i></span><span>analysis.py</span><button type="button" data-copy="import lzcomplexity as lz&#10;&#10;sequence = &quot;00010111&quot; * 256&#10;result = lz.lz76(sequence, log_base=2, max_block_size=16)&#10;&#10;print(result[&quot;h&quot;])&#10;print(result[&quot;emc&quot;][&quot;value&quot;])" aria-label="Copy Python example">Copy ⧉</button></div><pre><code><span class="lz-code-purple">import</span> lzcomplexity <span class="lz-code-purple">as</span> lz

<span class="lz-code-comment"># One symbolic sequence, several perspectives.</span>
sequence = <span class="lz-code-yellow">"00010111"</span> * <span class="lz-code-orange">256</span>
result = lz.<span class="lz-code-green">lz76</span>(
    sequence,
    log_base=<span class="lz-code-orange">2</span>,
    max_block_size=<span class="lz-code-orange">16</span>,
)

<span class="lz-code-green">print</span>(result[<span class="lz-code-yellow">"h"</span>])
<span class="lz-code-green">print</span>(result[<span class="lz-code-yellow">"emc"</span>][<span class="lz-code-yellow">"value"</span>])</code></pre><div class="lz-code-bottom">Explicit units. Explicit scales. Reproducible analysis.</div></div>
</section>

<section class="lz-applications">
<div class="lz-section-heading"><p class="lz-eyebrow">04 / FOLLOW YOUR CURIOSITY</p><h2>Different data.<br><em>Shared questions.</em></h2><p>Begin with a symbolic representation, then make the measurement choices explicit. These worked recipes show the path from data to analysis.</p></div>
<div class="lz-recipe-grid">
<a class="lz-recipe" href="recipes/neuro/"><svg viewBox="0 0 300 100" aria-hidden="true"><path d="M0 51H24L30 42 36 62 42 21 48 82 54 40 60 56 66 48H104L110 39 116 70 122 18 128 79 134 39 140 56 146 49H185L191 44 197 64 203 25 209 75 215 40 221 56 227 51H300"/></svg><span class="lz-eyebrow">SIGNALS → SYMBOLS</span><h3>Neural time series</h3><p>Choose a threshold, build a binary sequence and compare matched windows.</p><span class="lz-text-link">Explore the recipe ↗</span></a>
<a class="lz-recipe" href="recipes/genomics/"><div class="lz-dna-art" aria-hidden="true">A C <i>G</i> T A C G <i>T</i><br>T <i>G</i> C A T G <i>C</i> A<br>G C A <i>T</i> G <i>C</i> A T</div><span class="lz-eyebrow">BASES → PATTERNS</span><h3>Genomic sequences</h3><p>Read FASTA explicitly, handle ambiguous bases and preserve the chosen encoding.</p><span class="lz-text-link">Explore the recipe ↗</span></a>
<a class="lz-recipe" href="recipes/dynamical-systems/"><svg viewBox="0 0 300 100" aria-hidden="true"><path d="M12 85Q30 15 48 75T84 60 120 68 156 30 192 65 228 12 264 73 300 29"/><path d="M12 85Q45 10 78 56T144 38 210 54 276 20" opacity=".3"/></svg><span class="lz-eyebrow">DYNAMICS → ORGANIZATION</span><h3>Dynamical systems</h3><p>Symbolize a trajectory and examine length, transients and scale sensitivity.</p><span class="lz-text-link">Explore the recipe ↗</span></a>
</div>
</section>

<section class="lz-documentation-banner"><div><p class="lz-eyebrow">THE DETAILS MATTER</p><h2>Go from intuition<br><em>to a reproducible method.</em></h2><p>Concepts, implementation, every parameter and the original manual—connected in one documentation library.</p></div><div class="lz-documentation-links"><a href="concepts/implementation/">Inside the implementation <span>↗</span></a><a href="guide/reading-the-numbers/">Interpret the results <span>↗</span></a><a href="project/manual/">The original manual <span>↗</span></a><a href="project/references/">Papers &amp; references <span>↗</span></a></div></section>
<footer class="lz-home-footer"><span><strong>lzcomplexity</strong> <span class="lz-muted">/ by Pleros</span></span><p>Explore structure. Report your assumptions.</p><a href="https://github.com/pleros-ai/lzcomplexity">View on GitHub ↗</a></footer>
</div>
