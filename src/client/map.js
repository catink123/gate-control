/**
 * Apply CSS styles to a given element
 * @param {HTMLElement} el Element to apply styles to
 * @param {Object} styles CSS styles to apply
 */
function applyStyles(el, styles) {
    for (const [i, v] of Object.entries(styles)) {
        el.style[i] = v;
    }
}

/**
 * @typedef {Object} GateConfig
 * @property {number} x Gate's X position on the map
 * @property {number} y Gate's Y position on the map
 */

/**
 * @typedef {Object} ImageMapConfig
 * @property {string} mapImage URL to the image used in the map
 * @property {GateConfig[]} gates Existing gates
 */

const imageMapStyles = {
    container: {
        display: 'flex',
        position: 'relative',
        maxWidth: '1000px',
        width: 'fit-content',
        height: 'fit-content'
    },
    image: {
        width: '100%',
        height: '100%',
        left: 0,
        top: 0
    },
    bindingElement: {
        position: 'absolute',
        transform: 'translate(-50%, -50%)'
    }
};

/** A map of HTML elements confined to an image's dimensions */
class ImageMap {
    /**
     * Construct an ImageMap
     * @param {ImageMapConfig} config The configuration object for the map
     * @param {HTMLElement[]} bindingElements Elements to bind to the gate positions
     * @param {HTMLElement} container Element that will contain the new map
     */
    constructor(config, bindingElements, container) {
        /**
         * Stored configuration
         * @type {ImageMapConfig}
         */
        this.config = config;

        /**
         * Bound elements
         * @type {HTMLElement[]}
         */
        this.bindingElements = bindingElements;

        /**
         * Element to contain the map
         * @type {HTMLElement}
         */
        this.container = container;

        /**
         * Created image element
         * @type {HTMLImageElement}
         * @public
         */
        this.imageElement = document.createElement('img');
        applyStyles(this.imageElement, imageMapStyles.image);
        this.imageElement.addEventListener('load', () => this.#attach());
        this.imageElement.src = config.mapImage;
    }

    /**
     * Attach the map to the container
     */
    #attach() {
        /**
         * Container for the map image and the binding elements
         * @type {HTMLDivElement}
         * @public
         */
        this.workingElementsContainer = document.createElement('div');
        this.workingElementsContainer.className = 'image-map';
        applyStyles(this.workingElementsContainer, imageMapStyles.container);
        this.workingElementsContainer.appendChild(this.imageElement);
        for (const el of this.bindingElements) {
            this.workingElementsContainer.appendChild(el);
            applyStyles(el, imageMapStyles.bindingElement);
        }

        this.container.appendChild(this.workingElementsContainer);

        window.addEventListener('resize', () => this.#updatePositions());
        this.#updatePositions();
    }

    #updatePositions() {
        const sizeRatio = this.imageElement.width / this.imageElement.naturalWidth;
        for (const [i, el] of Object.entries(this.bindingElements)) {
            applyStyles(el, {
                left: this.config.gates[Number(i)].x * sizeRatio + 'px',
                top: this.config.gates[Number(i)].y * sizeRatio + 'px'
            });
        }
    }
}