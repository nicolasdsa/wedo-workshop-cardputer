(() => {
  const ANIMATE_CSS_BASE = "animate__animated";
  const ANIMATE_CSS_PREFIX = "animate__";
  const DEFAULT_DURATION_MS = 650;

  function normalizeAnimateCssName(name) {
    if (!name) return "";
    const trimmed = name.trim();
    if (!trimmed) return "";
    return trimmed.startsWith(ANIMATE_CSS_PREFIX) ? trimmed : `${ANIMATE_CSS_PREFIX}${trimmed}`;
  }

  function removeAnimateCssClasses(el) {
    if (!el) return;
    Array.from(el.classList).forEach((cls) => {
      if (cls === ANIMATE_CSS_BASE || cls.startsWith(ANIMATE_CSS_PREFIX)) {
        el.classList.remove(cls);
      }
    });
  }

  function parseAnimationKey(animationKey) {
    if (!animationKey) return { entry: "", exit: "" };
    const cleanedKey = animationKey.replace(/^animatecss:/i, "");
    const parts = cleanedKey.split(":").map((part) => part.trim());
    return {
      entry: parts[0] || "",
      exit: parts[1] || "",
    };
  }

  function replayAnimateCss(el, animationName, durationMs = DEFAULT_DURATION_MS) {
    if (!el || !animationName) return;
    const normalized = normalizeAnimateCssName(animationName);
    removeAnimateCssClasses(el);
    void el.offsetWidth;
    el.style.setProperty("--animate-duration", `${durationMs}ms`);
    el.classList.add(ANIMATE_CSS_BASE, normalized);
    el.addEventListener(
      "animationend",
      (event) => {
        if (event.target !== el) return;
        removeAnimateCssClasses(el);
      },
      { once: true }
    );
  }

  function waitAnimationEnd(el, timeoutMs = 1200) {
    return new Promise((resolve) => {
      if (!el) {
        resolve();
        return;
      }
      let settled = false;
      const done = () => {
        if (settled) return;
        settled = true;
        el.removeEventListener("animationend", onEnd);
        clearTimeout(timer);
        resolve();
      };
      const onEnd = (event) => {
        if (event.target !== el) return;
        done();
      };
      const timer = window.setTimeout(done, timeoutMs);
      el.addEventListener("animationend", onEnd);
    });
  }

  function applyAnimateCssAnimation(currentElement, nextElement, animationKey) {
    const { entry, exit } = parseAnimationKey(animationKey);
    const nextAnimRaw = entry;
    const currentAnimRaw = exit;
    const nextClass = normalizeAnimateCssName(nextAnimRaw);
    const currentClass = normalizeAnimateCssName(currentAnimRaw);

    if (!nextClass && !currentClass) {
      if (currentElement && currentElement !== nextElement) {
        currentElement.remove();
      }
      return true;
    }

    if (nextElement && nextClass) {
      nextElement.classList.add(ANIMATE_CSS_BASE, nextClass);
    }
    if (currentElement && currentClass) {
      currentElement.classList.add(ANIMATE_CSS_BASE, currentClass);
    }

    const cleanupTarget = (nextClass && nextElement) || currentElement;
    const cleanup = () => {
      if (nextElement && nextClass) {
        nextElement.classList.remove(ANIMATE_CSS_BASE, nextClass);
      }
      if (currentElement && currentClass) {
        currentElement.classList.remove(ANIMATE_CSS_BASE, currentClass);
      }
      if (currentElement && currentElement !== nextElement) {
        currentElement.remove();
      }
      if (cleanupTarget) {
        cleanupTarget.removeEventListener("animationend", cleanup);
      }
    };

    if (cleanupTarget) {
      cleanupTarget.addEventListener("animationend", cleanup, { once: true });
    } else {
      cleanup();
    }

    return true;
  }

  function applyScreenAnimation(currentElement, nextElement, animationKey) {
    if (animationKey) {
      const handled = applyAnimateCssAnimation(currentElement, nextElement, animationKey);
      if (handled) return;
    }
    if (currentElement && currentElement !== nextElement) {
      currentElement.remove();
    }
  }

  window.labAnimations = {
    normalizeAnimateCssName,
    parseAnimationKey,
    replayAnimateCss,
    waitAnimationEnd,
    applyScreenAnimation,
  };
})();
