import { routerService } from "./services/routerService.js";

routerService.setup(document.getElementById("content"));

routerService.addRoute("/", { partial: "/partials/home.html" });
routerService.addRoute("/home", { partial: "/partials/home.html" });
routerService.addRoute("/common-core", { partial: "/partials/common-core.html" });
routerService.addRoute("/specs", { partial: "/partials/specs.html" });

document.querySelectorAll("[data-route]").forEach((link) => {
	link.addEventListener("click", (e) => {
		e.preventDefault();
		const path = e.currentTarget.getAttribute("href");
		window.dispatchEvent(new CustomEvent("navigate", { detail: { path } }));
	});
});

routerService.start();