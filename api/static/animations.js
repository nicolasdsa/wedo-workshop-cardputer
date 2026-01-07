(() => {
  const ANIMATE_CSS_BASE = "animate__animated";
  const ANIMATE_CSS_PREFIX = "animate__";

  function normalizeAnimateCssName(name) {
    if (!name) return "";
    const trimmed = name.trim();
    if (!trimmed) return "";
    return trimmed.startsWith(ANIMATE_CSS_PREFIX) ? trimmed : `${ANIMATE_CSS_PREFIX}${trimmed}`;
  }

  function applyAnimateCssAnimation(currentElement, nextElement, animationKey) {
    const cleanedKey = animationKey.replace(/^animatecss:/i, "");
    const [nextAnimRaw, currentAnimRaw] = cleanedKey.split(":").map((part) => part.trim());
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
    applyScreenAnimation,
  };
})();
