new QWebChannel(qt.webChannelTransport, function(channel) {
    var context = channel.objects.connextChannel;
    window.context = context;

    console.log("Initing stuff");

    context.initialize.connect(function(seq, payload) {
        window.initialize(seq, payload)
    })
    context.setup.connect(function(seq, payload) {
        window.setup(seq, payload)
    })
    context.getConfig.connect(function(seq) {
        window.getConfig(seq)
    })
    context.getStateChannels.connect(function(seq) {
        window.getStateChannels(seq)
    })
    context.getStateChannel.connect(function(seq, payload) {
        window.getStateChannel(seq, payload)
    })
    context.conditionalTransfer.connect(function(seq, payload) {
        window.conditionalTransfer(seq, payload)
    })
    context.resolveTransfer.connect(function(seq, payload) {
        window.resolveTransfer(seq, payload)
    })
    context.reconcileDeposit.connect(function(seq, payload) {
        window.reconcileDeposit(seq, payload)
    })
    context.withdrawDeposit.connect(function(seq, payload) {
        window.withdrawDeposit(seq, payload)
    })
    context.sendDepositTx.connect(function(seq, payload) {
        window.sendDepositTx(seq, payload)
    })
    context.restoreState.connect(function(seq, payload) {
        window.restoreState(seq, payload)
    })
    context.getTransfers.connect(function(seq, payload) {
        window.getTransfers(seq, payload)
    })
});
