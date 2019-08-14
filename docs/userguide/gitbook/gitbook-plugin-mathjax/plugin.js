require(["gitbook"], function(gitbook) {
    MathJax.Hub.Config({
        tex2jax: {}
    });


    gitbook.events.bind("page.change", function() {
        MathJax.Hub.Typeset()
    });
});

